// TDConcept.cpp: implementation of the CTDConcept class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDCONCEPT_H)
    #include "TDConcept.h"
#endif

#if !defined(TDPARTITION_H)
    #include "TDPartition.h"
#endif

//**************
// CTDConcepts *
//**************
CTDConcepts::CTDConcepts()
{
}

CTDConcepts::~CTDConcepts()
{
}

void CTDConcepts::cleanup()
{
    for (int i = 0; i < GetSize(); ++i)
        delete GetAt(i);

    RemoveAll();
}

CString CTDConcepts::toString()
{
	CString str;
	str += GetAt(0)->toString();
	for (int i = 1; i < GetSize(); ++i){
		str += _T("-");
		str += GetAt(i)->toString();
	}		
    return str;
}

//*************
// CTDConcept *
//*************

CTDConcept::CTDConcept(CTDAttrib* pAttrib)
    : m_pParentConcept(NULL), 
      m_pAttrib(pAttrib),
      m_childIdx(-1), 
      m_flattenIdx(-1), 
      m_depth(-1),
      m_bCutCandidate(true),
      m_cutPos(NULL),
	  m_bFileName(false),
	  m_nLeafConcepts(-1)
     
{
}

CTDConcept::~CTDConcept() 
{
	m_childConcepts.cleanup();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDConcept::addChildConcept(CTDConcept* pConceptNode)
{
    try {
        pConceptNode->m_pParentConcept = this;
        pConceptNode->m_childIdx = m_childConcepts.Add(pConceptNode);	
        return true;
    }
    catch (CMemoryException&) {
		cout << "This is error#1." << endl;
		cout << "m_childConcepts size: " <<  m_childConcepts.GetSize() << endl;
        ASSERT(false);
        return false;
    }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDConcept::addChildConcept(CTDConcept* pConceptNode, int idx)	
{
    try {
        pConceptNode->m_pParentConcept = this;
        m_childConcepts.Add(pConceptNode);	
		pConceptNode->m_childIdx = idx;
        return true;
    }
    catch (CException& exObj) {
		char* errMsg = NULL;
		exObj.GetErrorMessage(errMsg, 1024);
		cout << "This is error#2." << endl;
		cout << "m_childConcepts size: " <<  m_childConcepts.GetSize() << endl;
		cout << "Error message is: " << endl;
		cout << errMsg << endl;
        ASSERT(false);
        return false;
    }
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int CTDConcept::getNumChildConcepts() const
{ 
    return m_childConcepts.GetSize(); 
}

//---------------------------------------------------------------------------
// Return the number of leaf concepts of the tree rooted at the this concept
//---------------------------------------------------------------------------
int CTDConcept::getNumLeafConcepts()
{ 
	int nLeaves = 0;
	if (getNumChildConcepts() == 0)
		return 1;
	else {
		CTDConcept* pChildConcept = NULL;
		for (int i = 0; i < getNumChildConcepts(); ++i) {
			pChildConcept = getChildConcept(i);
			nLeaves += pChildConcept->getNumLeafConcepts();
		}
	}
	return nLeaves;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDConcept* CTDConcept::getChildConcept(int idx) const
{
    return m_childConcepts.GetAt(idx); 
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDConcept* CTDConcept::getParentConcept() 
{ 
    return m_pParentConcept; 
}

//---------------------------------------------------------------------------
// Compute NCP for this concept.
// Please refer to our paper.
//---------------------------------------------------------------------------
bool CTDConcept::computeNCPHelper(float& ncp)
{
	if (this->isContinuous()) {
		cerr << _T("CTDConcept::computeNCPHelper(): must be categorical.") << endl;
        ASSERT(false);
        return false;
	}

	ncp = 0;
	
	int nCurrLeafConcepts = this->getNumLeafConcepts();	// Rooted at this concept.
	int nTotalLeafConcepts = this->getAttrib()->getConceptRoot()->m_nLeafConcepts;
	if (nCurrLeafConcepts == 1) 
		ncp = 0;
	else 
		ncp = float (nCurrLeafConcepts * 1.0 / nTotalLeafConcepts);

    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDConcept::parseFirstConcept(CString& firstConcept, CString& restStr)
{
    firstConcept.Empty();
    int len = restStr.GetLength();
    if (len < 2 ||
        restStr[0] !=  TD_CONHCHY_OPENTAG || 
        restStr[len - 1] !=  TD_CONHCHY_CLOSETAG) {
		ASSERT(false);
        return false;
    }

    // Find the index number of the closing tag of the first concept
    int tagCount = 0;
    for (int i = 0; i < len; ++i) {
        if (restStr[i] == TD_CONHCHY_OPENTAG)
            ++tagCount;
        else if (restStr[i] == TD_CONHCHY_CLOSETAG) {
            --tagCount;
            ASSERT(tagCount >= 0);
            if (tagCount == 0) {
                // Closing tag of first concept found
                firstConcept = restStr.Left(i + 1);
                restStr = restStr.Mid(i + 1);
                CBFStrHelper::trim(restStr);
                return true;
            }
        }
    }
    ASSERT(false);
    return false;
}

//*****************
// CTDDiscConcept *
//*****************

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDDiscConcept::CTDDiscConcept(CTDAttrib* pAttrib) 
    : CTDConcept(pAttrib), m_pSplitConcept(NULL)
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDDiscConcept::~CTDDiscConcept() 
{
}

//---------------------------------------------------------------------------
// {Any_Location {BC {Vancouver} {Surrey} {Richmond}} {AB {Calgary} {Edmonton}}}
//---------------------------------------------------------------------------
bool CTDDiscConcept::initHierarchy(LPCTSTR conceptStr, int depth, CTDIntArray& maxBranches, int& maxDepth)
{
    // Parse the conceptValue and the rest of the string
    CString restStr;
    if (!parseConceptValue(conceptStr, m_conceptValue, restStr)) {
        cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << conceptStr << endl;
        return false;
    }
    m_depth = depth;

	// Update maxDepth
	depth > maxDepth ? maxDepth = depth : maxDepth;

    // Depth-first build
    CString firstConcept;
    while (!restStr.IsEmpty()) {
        if (!parseFirstConcept(firstConcept, restStr)) {
            cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << restStr << endl;
            return false;
        }

        CTDDiscConcept* pNewConcept = new CTDDiscConcept(m_pAttrib);
        if (!pNewConcept)
            return false;
        
        if (!pNewConcept->initHierarchy(firstConcept, depth + 1, maxBranches, maxDepth)) {
            cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << firstConcept << endl;
            return false;
        }

        if (!addChildConcept(pNewConcept))
            return false;
    }

    // Update the maximum # of branches at this level
    int nChildren = getNumChildConcepts();
    if (nChildren > 0) {
        while (depth > maxBranches.GetUpperBound())	
            maxBranches.Add(0);
        if (nChildren > maxBranches[depth])			
            maxBranches[depth] = nChildren;		 
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CString CTDDiscConcept::toString()
{
    return m_conceptValue;
}


//---------------------------------------------------------------------------
// Return true if pTargetConcept is an ancestor of this concept
//---------------------------------------------------------------------------
bool CTDDiscConcept::isAncestor(CTDConcept* pTargetConcept)
{
	if (this->m_conceptValue == pTargetConcept->m_conceptValue) {
#ifdef _DEBUG_PRT_INFO
		cout << "Error finding ancestor. Current and target concepts are the same.\n";
#endif
		return false;
	}

	CTDConcept* pAncestor = this->getParentConcept();
	while (pAncestor)
	{
		if (pAncestor->m_conceptValue == pTargetConcept->m_conceptValue) 
			return true;
		pAncestor = pAncestor->getParentConcept();
	}

	return false;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDDiscConcept::parseConceptValue(LPCTSTR str, CString& conceptVal, CString& restStr)
{
    conceptVal.Empty();
    restStr.Empty();

    CString wrkStr = str;
    if (wrkStr.GetLength() < 2 ||
        wrkStr[0] !=  TD_CONHCHY_OPENTAG || 
        wrkStr[wrkStr.GetLength() - 1] !=  TD_CONHCHY_CLOSETAG) {
        ASSERT(false);
        return false;
    }

    // Extract "Canada {BC {Vancouver} {Surrey} {Richmond}} {AB {Calgary} {Edmonton}}"
    wrkStr = wrkStr.Mid(1, wrkStr.GetLength() - 2);
    CBFStrHelper::trim(wrkStr);
    if (wrkStr.IsEmpty()) {
        ASSERT(false);
        return false;
    }

    // Extract "Canada"
    int openPos = wrkStr.Find(TD_CONHCHY_OPENTAG);
    if (openPos < 0) {
        // This is root value, e.g., "Vancouver"
        conceptVal = wrkStr;
        return true;
    }
    else {
        conceptVal = wrkStr.Left(openPos);
        CBFStrHelper::trim(conceptVal);
        if (conceptVal.IsEmpty()) {
            ASSERT(false);
            return false;
        }
    }

    // Extract "{BC {Vancouver} {Surrey} {Richmond}} {AB {Calgary} {Edmonton}}"
    restStr = wrkStr.Mid(openPos);
    return true;
}

//*****************
// CTDContConcept *
//*****************

CTDContConcept::CTDContConcept(CTDAttrib* pAttrib) 
    : CTDConcept(pAttrib), 
		m_lowerBound(0.0f), 
		m_upperBound(0.0f)	
{
}

CTDContConcept::~CTDContConcept() 
{
}

//---------------------------------------------------------------------------
// {0-100 {0-50 {<25} {25-50}} {50-100 {50-75} {75-100}}}
//---------------------------------------------------------------------------
bool CTDContConcept::initHierarchy(LPCTSTR conceptStr, int depth, CTDIntArray& maxBranches, int& maxDepth)
{
    // Parse the conceptValue and the rest of the string.
    CString restStr;
    if (!parseConceptValue(conceptStr, m_conceptValue, restStr, m_lowerBound, m_upperBound)) {
        cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << conceptStr << endl;
        return false;
    }

#ifdef _TD_MANUAL_CONTHRCHY

	m_depth = depth;
	
	// Update maxDepth
	depth > maxDepth ? maxDepth = depth : maxDepth;

    // Depth-first build.
    CString firstConcept;
    while (!restStr.IsEmpty()) {
        if (!parseFirstConcept(firstConcept, restStr)) {
            cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << restStr << endl;
            return false;
        }
		
		
        CTDContConcept* pNewConcept = new CTDContConcept(m_pAttrib);
        if (!pNewConcept)
            return false;
        
		
		if (!pNewConcept->initHierarchy(firstConcept, depth + 1, maxBranches, maxDepth)) {
            cerr << _T("CTDDiscConcept: Failed to build hierarchy from ") << firstConcept << endl;
            return false;
        }

        if (!addChildConcept(pNewConcept))
            return false;
    }
#endif

    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CString CTDContConcept::toString()
{
#ifdef _TD_TREAT_CONT_AS_CONT
    CString str;
    LPTSTR tempStr = this->FloatToStr((m_lowerBound + m_upperBound) / 2.0f, TD_CONTVALUE_NUMDEC);
    str = tempStr;
    delete [] tempStr;
    return str;    
#else
    CString str;
    LPTSTR tempStr = this->FloatToStr(m_lowerBound, TD_CONTVALUE_NUMDEC);
    str += tempStr;
    str += _T("-");
    delete [] tempStr;
    tempStr = this->FloatToStr(m_upperBound, TD_CONTVALUE_NUMDEC);
    str += tempStr;
    delete [] tempStr;
    tempStr = NULL;
    return str;
#endif
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDContConcept::parseConceptValue(LPCTSTR str, 
                                       CString& conceptVal, 
                                       CString& restStr, 
                                       float& lowerBound, 
                                       float& upperBound)
{
    conceptVal.Empty();
    restStr.Empty();
    lowerBound = 0.0f;
    upperBound = 0.0f;

    CString wrkStr = str;
    if (wrkStr.GetLength() < 2 ||
        wrkStr[0] !=  TD_CONHCHY_OPENTAG || 
        wrkStr[wrkStr.GetLength() - 1] !=  TD_CONHCHY_CLOSETAG) {
        ASSERT(false);
        return false;
    }

    wrkStr = wrkStr.Mid(1, wrkStr.GetLength() - 2);
    CBFStrHelper::trim(wrkStr);
    if (wrkStr.IsEmpty()) {
        ASSERT(false);
        return false;
    }

    // Extract "0-100"
    LPTSTR tempStr = NULL;
    int openPos = wrkStr.Find(TD_CONHCHY_OPENTAG);
    if (openPos < 0) {
        // This is root value, e.g., "0-50"
        conceptVal = wrkStr;
        if (!parseLowerUpperBound(conceptVal, lowerBound, upperBound))
            return false;
    }
    else {
        conceptVal = wrkStr.Left(openPos);
        CBFStrHelper::trim(conceptVal);
        if (conceptVal.IsEmpty()) {
            ASSERT(false);
            return false;
        }

        if (!parseLowerUpperBound(conceptVal, lowerBound, upperBound))
            return false;
    }

    // Convert again to make sure exact match for decimal places
    if (!makeRange(lowerBound, upperBound, conceptVal)) {
        ASSERT(false);
        return false;
    }

    restStr = wrkStr.Mid(openPos);
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDContConcept::makeRange(float lowerB, float upperB, CString& range)
{
    LPTSTR tempStr = FloatToStr(lowerB, TD_CONTVALUE_NUMDEC);
    range = tempStr;
    delete [] tempStr;
    tempStr = NULL;
    tempStr = FloatToStr(upperB, TD_CONTVALUE_NUMDEC);
    range += _T("-");
    range += tempStr;
    delete [] tempStr;
    tempStr = NULL;
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// static
bool CTDContConcept::parseLowerUpperBound(const CString& str, float& lowerB, float& upperB)
{
    lowerB = upperB = 0.0f;

    int dashPos = str.Find(TD_CONHCHY_DASHSYM);
    if (dashPos < 0) {
        cerr << _T("CTDDiscConcept: Failed to parse ") << str << endl;
        ASSERT(false);
        return false;
    }

    CString lowStr = str.Left(dashPos);
    CBFStrHelper::trim(lowStr);
    if (lowStr.IsEmpty()) {
        cerr << _T("CTDDiscConcept: Failed to parse ") << str << endl;
        ASSERT(false);
        return false;
    }
    
    CString upStr = str.Mid(dashPos + 1);
    CBFStrHelper::trim(upStr);
    if (upStr.IsEmpty()) {
        cerr << _T("CTDDiscConcept: Failed to parse ") << str << endl;
        ASSERT(false);
        return false;
    }

    lowerB = (float) StrToFloat(lowStr);
    upperB = (float) StrToFloat(upStr);
    if (lowerB > upperB) {
        ASSERT(false);
        return false;
    }
    return true;
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDContConcept::makeChildConcepts(float splitPoint, CTDConcept*& pLChildCon, CTDConcept*& pRChildCon)
{
	// No split if all raw values are the same
	if (splitPoint == FLT_MAX)
		return true;

    // Make left child
    CTDContConcept* pLeftConcept = new CTDContConcept(m_pAttrib);
    if (!pLeftConcept) {
        ASSERT(false);
        return false;
    }
    pLeftConcept->m_lowerBound = m_lowerBound;
    pLeftConcept->m_upperBound = splitPoint;

	try {
		pLeftConcept->m_flattenIdx = m_pAttrib->getFlattenConcepts()->Add(pLeftConcept);
	}
	catch (CException& exObj) {
		char* errMsg = NULL;
		exObj.GetErrorMessage(errMsg, 1024);
		cout << errMsg << endl;
        ASSERT(false);
        return false;
    }

    LPTSTR tempStr = this->FloatToStr(m_lowerBound, TD_CONTVALUE_NUMDEC);
    pLeftConcept->m_conceptValue = tempStr;
    pLeftConcept->m_conceptValue += _T("-");    
    delete [] tempStr;
    tempStr = this->FloatToStr(splitPoint, TD_CONTVALUE_NUMDEC);
    pLeftConcept->m_conceptValue += tempStr;
    delete [] tempStr;
    tempStr = NULL;    
	pLChildCon = pLeftConcept;
    if (!addChildConcept(pLeftConcept, 0))
        return false;
	

    // Make right child
    CTDContConcept* pRightConcept = new CTDContConcept(m_pAttrib);
    if (!pRightConcept) {
        ASSERT(false);
        return false;
    }
    pRightConcept->m_lowerBound = splitPoint;
    pRightConcept->m_upperBound = m_upperBound;

	try {
		pRightConcept->m_flattenIdx = m_pAttrib->getFlattenConcepts()->Add(pRightConcept);
	}
	catch (CException& exObj) {
		char* errMsg;
		exObj.GetErrorMessage(errMsg, 1024);
		cout << errMsg << endl;
        ASSERT(false);
        return false;
    }

    tempStr = this->FloatToStr(splitPoint, TD_CONTVALUE_NUMDEC);
    pRightConcept->m_conceptValue = tempStr;
    pRightConcept->m_conceptValue += _T("-");
    delete [] tempStr;
    tempStr = this->FloatToStr(m_upperBound, TD_CONTVALUE_NUMDEC);
    pRightConcept->m_conceptValue += tempStr;
    delete [] tempStr;
    tempStr = NULL;
	pRChildCon = pRightConcept;
    if (!addChildConcept(pRightConcept, 1)) 
        return false;
	
    return true;
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
char * CTDContConcept::FloatToStr (double value, Int16s nDecimals)
{
    char numericString [100];

    sprintf_s (numericString, "%.*f", nDecimals, value);

    char * string = new char [lstrlen (numericString) + 1];

    if (string != 0)
        lstrcpy (string, numericString);

    return string;
}
