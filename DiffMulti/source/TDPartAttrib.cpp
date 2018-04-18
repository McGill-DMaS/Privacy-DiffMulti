// TDPartAttrib.cpp: implementation of the CTDPartAttrib class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDPARTATTRIB_H)
    #include "TDPartAttrib.h"
#endif

#if !defined(TDPARTITION_H)
    #include "TDPartition.h"
#endif

//****************
// CTDPartAttrib *
//****************

CTDPartAttrib::CTDPartAttrib(CTDAttrib* pActualAttrib)
    : m_pActualAttrib(pActualAttrib), 
      m_pSupportMatrix(NULL), 
      m_bCandidate(true),
	  m_infoGain(-1.0f), 
	  m_max(-1.0f),
	  m_discern(0LL),
	  m_ncp(-1.0f),
	  m_totalNCP(-1.0f),
	  m_splitPoint(FLT_MAX),
	  m_pLeftChildCon(NULL),
	  m_pRightChildCon(NULL),
	  m_pSplitSupMatrix(NULL)
{
}

CTDPartAttrib::~CTDPartAttrib() 
{
    delete m_pSupportMatrix;

	delete m_pSplitSupMatrix;
    m_pSplitSupMatrix = NULL;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartAttrib::initSupportMatrix(CTDConcept* pCurrCon, int nClasses)
{
	int nChildConcepts;
	if (pCurrCon->isContinuous())
		nChildConcepts = 2;
	else
		nChildConcepts = pCurrCon->getNumChildConcepts();

    // Allocate the matrix
    int dims[] = {nChildConcepts, nClasses};
    m_pSupportMatrix = new CTDMDIntArray(sizeof(dims) / sizeof(int), dims);
    if (!m_pSupportMatrix) {
        ASSERT(false);
        return false;
    }

    // Allocate support sums
    m_supportSums.SetSize(nChildConcepts);

    // Allocate class sums
    m_classSums.SetSize(nClasses);

    for (int i = 0; i < nChildConcepts; ++i) {
        for (int j = 0; j < nClasses; ++j) {
            (*m_pSupportMatrix)[i][j] = 0;
            m_classSums.SetAt(j, 0);
        }
        m_supportSums.SetAt(i, 0);
    }
    return true;
}

//---------------------------------------------------------------------------
// Compute the score of current concept in this partAttrib
//---------------------------------------------------------------------------
bool CTDPartAttrib::computeScore(int nClasses, CTDConcept* pCurrCon)
{

	if (m_infoGain != -1.0f || !pCurrCon->m_bCutCandidate)
		return true;

	if (!m_bCandidate) {
#ifdef _DEBUG_PRT_INFO
		cout << pCurrCon->m_conceptValue << " is not m_bCandidate" << endl << endl;
#endif
		if (!pCurrCon->isContinuous()) 
			pCurrCon->m_bCutCandidate = false;
		return true;
	} 
        
	// counts for the current partition
	int nChildConcepts;
	if (pCurrCon->isContinuous())
		nChildConcepts = 2;
	else
		nChildConcepts = pCurrCon->getNumChildConcepts();

	ASSERT(getSupportSums()->GetSize() == nChildConcepts);  
	if (!computeMaxHelper(*getSupportSums(), *getClassSums(), *getSupportMatrix(), m_max)) {
		ASSERT(false);
		return false;
	}
	if (!computeInfoGainHelper(computeEntropy(getClassSums()), *getSupportSums(), *getClassSums(), *getSupportMatrix(), m_infoGain)) {
		ASSERT(false);
		return false;
	}
	if (!computeDiscernHelper(*getSupportSums(), m_discern)) {
		ASSERT(false);
		return false;
	}
	if (!computeNCPHelper(*getSupportSums(), m_ncp, pCurrCon)) {
		ASSERT(false);
		return false;
	}
	
    return true;
}

//---------------------------------------------------------------------------
// Compute Max count for this partAttrib
//---------------------------------------------------------------------------
// static
bool CTDPartAttrib::computeMaxHelper(const CTDIntArray& supSums, 
									const CTDIntArray& classSums,
									CTDMDIntArray& supMatrix,
									float& max)
{
    max = 0.0f;
	int cMax = 0, totalMax = 0;
    int s = 0, c = 0;
    
	int nSupports = supSums.GetSize();    
	int nClasses = classSums.GetSize();

	for (s = 0; s < nSupports; ++s) {
        cMax = 0;
		for (c = 0; c < nClasses; ++c) {
    		if ( supMatrix[s][c] > cMax) {
                cMax = supMatrix[s][c];
            }
		} 
		totalMax = totalMax + cMax;
   	}
    
	max = (float) totalMax;

    return true;
}

//---------------------------------------------------------------------------
// Compute information gain for this partAttrib
//---------------------------------------------------------------------------
// static
bool CTDPartAttrib::computeInfoGainHelper(float entropy, 
                                       const CTDIntArray& supSums, 
                                       const CTDIntArray& classSums, 
                                       CTDMDIntArray& supMatrix,
                                       float& infoGainDiff)
{
    infoGainDiff = 0.0f;
    int total = 0, s = 0;
    int nSupports = supSums.GetSize();    
    for (s = 0; s < nSupports; ++s)
        total += supSums.GetAt(s);
    
	if (total == 0)
		return true;

//	ASSERT(total > 0);

    int nClasses = classSums.GetSize();
    int c = 0;
    float r = 0.0f, mutualInfo = 0.0f, infoGainS = 0.0f;
    for (s = 0; s < nSupports; ++s) {
        infoGainS = 0.0f;
        for (c = 0; c < nClasses; ++c) {
            //ASSERT(supSums->GetAt(s) > 0); 
            r = float(supMatrix[s][c]) / supSums.GetAt(s);
            if (r > 0.0f) 
                infoGainS += (r * this->log2f(r)) * -1; 
        }        
        mutualInfo += (float(supSums.GetAt(s)) / total) * infoGainS;
    }
    infoGainDiff = entropy - mutualInfo;
    return true;
}

//---------------------------------------------------------------------------
// Compute Discernibility for this partAttrib
//---------------------------------------------------------------------------
bool CTDPartAttrib::computeDiscernHelper(const CTDIntArray& supSums, long long& discern)
{
	discern = 0;
	int s = 0;
	int nSupport = supSums.GetSize();
	for (int s = 0; s < nSupport; ++s) 
		discern += square(supSums.GetAt(s));

    return true;
}

//---------------------------------------------------------------------------
// Compute entropy
//---------------------------------------------------------------------------
// static
float CTDPartAttrib::computeEntropy(CTDIntArray* pClassSums) 
{
    float entropy = calEntropy(pClassSums);

    return entropy;
}


//---------------------------------------------------------------------------
// Compute NCP when choosing a split point.
// For numerical attributes, we define ncp as:
// ncp(interval) = |interval| / |root interval|.
//			
// ncp(numConcept) = [number of records * ncp(interval)].
// The ncp of a parent numerical concept = ncp(LeftChildConcept) +  ncp(RightChildConcept).
//
// The midpoint of an interval is used to represent its ncp.
//
// All the values within the interval have the same supSums[]. 
//---------------------------------------------------------------------------
bool CTDPartAttrib::computeNCPSplitHelper(const CTDIntArray& supSums, float& ncp, CTDNumericValue* pCurrValue, CTDNumericValue* pNextValue, CTDContConcept*  pCurrConcept)
{
	ncp = 0;
	float midpoint = (pNextValue->getRawValue() + pCurrValue->getRawValue()) / 2;
	CTDContConcept* pNumRootConcept = static_cast <CTDContConcept*> (pCurrConcept->getAttrib()->getConceptRoot());
	float rootRange = pNumRootConcept->m_upperBound - pNumRootConcept->m_lowerBound;

	// ncp of left child concept
	float leftRange = midpoint - pCurrConcept->m_lowerBound;
	float leftNCP = (supSums[0]) * (leftRange / rootRange);

	// ncp of right child concept
	float rightRange = pCurrConcept->m_upperBound - midpoint;
	float rightNCP = (supSums[1]) * (rightRange / rootRange);
	
	// return sum 
	ncp = leftNCP + rightNCP;

	return true;
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartAttrib::computeNCPHelper(const CTDIntArray& supSums, float& ncp, CTDConcept* pCurrCon)
{
	// Just like in Max and InfoGain, ncp = sum of ncp's of child concepts.
	ncp = 0;
	if (pCurrCon->getNumChildConcepts() == 0) {
		cerr << _T("CTDPartAttrib::computeNCPHelper(): no child concepts.") << endl;
		cerr << pCurrCon->getAttrib()->m_attribName << ": " << pCurrCon->m_conceptValue << endl;
        ASSERT(false);
        return false;
	}
	else {
		if (this->getActualAttrib()->isContinuous()) {
			CTDContConcept*  pCurrConcept = static_cast<CTDContConcept*> (pCurrCon);
#ifdef _DEBUG_PRT_INFO
			cout << pCurrCon->getAttrib()->m_attribName << endl;
			cout << "Parent interval     : [" << pCurrConcept->m_lowerBound << "-" << pCurrConcept->m_upperBound << "]" << endl;
			cout << "Split point         : " << m_splitPoint << endl;
#endif
			CTDConcept* pChildConcept = NULL;
			float childNCP = 0;
			computeNCPHelperHelper(childNCP, this->m_pLeftChildCon);
			ncp += (supSums.GetAt(0) * childNCP);
#ifdef _DEBUG_PRT_INFO
			cout << "      (" << supSums.GetAt(0) << " * " << childNCP << ")" << endl;
#endif
			childNCP = 0;
			computeNCPHelperHelper(childNCP, this->m_pRightChildCon);
			ncp += (supSums.GetAt(1) * childNCP);
#ifdef _DEBUG_PRT_INFO
			cout << " +    (" << supSums.GetAt(1) << " * " << childNCP << ") = " << ncp << endl;
#endif
			
		}
		else {
#ifdef _DEBUG_PRT_INFO
			cout << pCurrCon->getAttrib()->m_attribName << endl;
			cout << "Parent concept      : " << pCurrCon->m_conceptValue << endl;
#endif
			CTDConcept* pChildConcept = NULL;
			for (int i = 0; i < pCurrCon->getNumChildConcepts(); ++i) {
				float childNCP = 0;
				computeNCPHelperHelper(childNCP, pCurrCon->getChildConcept(i));
				ncp += (supSums.GetAt(i) * childNCP);
#ifdef _DEBUG_PRT_INFO
				cout << " +    (" << supSums.GetAt(i) << " * " << childNCP << ")" << endl;
#endif
			}
#ifdef _DEBUG_PRT_INFO
			cout << " = " << ncp << endl;
#endif
		}
#ifdef _DEBUG_PRT_INFO
		cout << endl;
#endif
	}

	return true;
}

//---------------------------------------------------------------------------
// Compute NCP of pCurrCon.
// If the concept is continuous: ncp = |interval| / |rootInterval|
// If categorical, please refer to the paper.
//---------------------------------------------------------------------------
bool CTDPartAttrib::computeNCPHelperHelper(float& ncp, CTDConcept* pCurrCon)
{
	ncp = 0;

	if (this->getActualAttrib()->isContinuous()) {
		CTDContConcept*  pCurrConcept = static_cast<CTDContConcept*> (pCurrCon);
		CTDContConcept* pNumRootConcept = static_cast <CTDContConcept*> (pCurrConcept->getAttrib()->getConceptRoot());
#ifdef _DEBUG_PRT_INFO
		cout << "Child interval      : [" << pCurrConcept->m_lowerBound << "-" << pCurrConcept->m_upperBound << "]" << endl;
#endif
		ncp = (pCurrConcept->m_upperBound - pCurrConcept->m_lowerBound) / (pNumRootConcept->m_upperBound - pNumRootConcept->m_lowerBound);
	}

	else {
#ifdef _DEBUG_PRT_INFO
		cout << "Child concept       : " << pCurrCon->m_conceptValue << endl;
#endif
		int nTotalLeafConcepts = this->getActualAttrib()->getConceptRoot()->m_nLeafConcepts;
		int nCurrLeafConcepts = pCurrCon->getNumLeafConcepts();	// Number of leaves under this concept.
		if (nCurrLeafConcepts == 1) 
			ncp = 0;
		else 
			ncp = float (nCurrLeafConcepts * 1.0 / nTotalLeafConcepts);
	}
	
    return true;
}

//---------------------------------------------------------------------------
// Split a continuous concept:
// 1) Sort partRecords based on raw values of the continuous attribute.
// 2) Find the optimal split point.
// 3) Add the child concepts to this concept.
//---------------------------------------------------------------------------
bool CTDPartAttrib::divideConcept(double epsilon, int nClasses, CTDConcept* pCurrConcept, CTDPartition* pCurrPartition)
{
   #ifdef _TD_MANUAL_CONTHRCHY
		
		return true;

	#endif

	if (!pCurrConcept->isContinuous())
		return true;

	// Cannot split on this partAttrib
	if (!m_bCandidate)
		return true;

	// Child nodes created in a previous iteration
	// No need to divide again
	if (m_splitPoint != FLT_MAX) {
#ifdef _DEBUG_PRT_INFO
		cout << "PartAttrib			: " << m_pActualAttrib->m_attribName << endl;
		cout << "pCurrConcept			: " << pCurrConcept->m_conceptValue << endl;
		cout << "# of child concepts		: " << pCurrConcept->getNumChildConcepts() << endl;
		cout << "# of partition records		: " << pCurrPartition->getNumRecords() << endl;
		cout << "m_splitPoint			: " << m_splitPoint << endl;
		cout << "m_pLeftChildCon			: " << m_pLeftChildCon->m_conceptValue << endl;
		cout << "m_pRightChildCon		: " << m_pRightChildCon->m_conceptValue << endl << endl;
#endif
        return true;
	}

	CTDContConcept* pParentContConcept = NULL;
	CTDContConcept* pLChildContConcept = NULL;
	pParentContConcept = static_cast<CTDContConcept*> (pCurrConcept);

	if (pParentContConcept->getNumChildConcepts() != 0) { 
		pLChildContConcept = static_cast<CTDContConcept*> (pParentContConcept->getChildConcept(0));
		m_splitPoint = pLChildContConcept->m_upperBound;
#ifdef _DEBUG_PRT_INFO
		cout << "A split point already exists." << endl;
		cout << "PartAttrib			: " << m_pActualAttrib->m_attribName << endl;
		cout << "pCurrConcept			: " << pCurrConcept->m_conceptValue << endl;
		cout << "m_splitPoint			: " << m_splitPoint << endl;
		cout << "Left child			: " << pParentContConcept->getChildConcept(0)->m_conceptValue << endl;
		cout << "Right child			: " << pParentContConcept->getChildConcept(1)->m_conceptValue << endl << endl;
#endif
		m_pLeftChildCon = pParentContConcept->getChildConcept(0);
		m_pRightChildCon = pParentContConcept->getChildConcept(1);
		return true;
	}
		
	// Split the continuous concept
    CTDRecords* Recs = pCurrPartition->getAllRecords();
    if (Recs->GetSize() <= 1) {    
        // srand( (unsigned)time( NULL ) );
		m_splitPoint = (pParentContConcept->m_upperBound + pParentContConcept->m_lowerBound) / 2;
	}
	else {
		 // Sort the recrods according to their raw values of this attribute
         if (!Recs->sortByAttrib(m_pActualAttrib->m_attribIdx))
             return false;

         // Find optimal split point
         if (!findOptimalSplitPoint(*Recs, nClasses, epsilon, pCurrConcept))
             return false;
	}

	// Add the child concepts to this concept
	if (!pParentContConcept->makeChildConcepts(m_splitPoint, m_pLeftChildCon, m_pRightChildCon))
		return false;

    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartAttrib::findOptimalSplitPoint(CTDRecords& recs, int nClasses, double epsilon, CTDConcept* pCurrConcept)
{
	// Initializing m_pSplitSupMatrix: # of dimensions.
	// All indexes are set to 0
	// A continuous attribute has 2 child nodes at max 
    if (!initSplitMatrix(2, nClasses))
        return false;

    if (!m_pActualAttrib->isContinuous()) {
        ASSERT(false);
        return false;
    }

    int attribIdx = m_pActualAttrib->m_attribIdx;
    int classIdx = recs.GetAt(0)->getNumValues() - 1;
    int nRecs = recs.GetSize();

    // Initialize counters
	int r = 0;
    CTDRecord* pCurrRec = NULL;
    CTDRecord* pNextRec = NULL;
    CTDConcept* pClassConcept = NULL;
    for (r = 0; r < nRecs; ++r) {
        pCurrRec = recs.GetAt(r);
        pClassConcept = pCurrRec->getValue(classIdx)->getCurrentConcept();
        ++((*m_pSplitSupMatrix)[1][pClassConcept->m_childIdx]);	//** Increment on right child interval.
        ++(m_splitSupSums[1]);
        ++(m_splitClassSums[pClassConcept->m_childIdx]);
    }
    
	CTDRanges cRanges; 
	cRanges.cleanup();
	CTDFloatArray weights;
	CTDFloatArray ranges;

	int idx = 0;
	float entropy = 0.0f;
    float infoGain = 0.0f;
	float max = 0.0f;
	long long discern = 0;
	float ncp = 0.0f;
    bool FLAG = false;
    
	CTDNumericValue* pCurrValue		= NULL;
    CTDNumericValue* pNextValue		= NULL;
	CTDContConcept*  pContConcept	= NULL;
	pContConcept = static_cast<CTDContConcept*> (recs.GetAt(0)->getValue(attribIdx)->getCurrentConcept());
    for (r = 0; r < nRecs - 1; ++r) {
        pCurrRec = recs.GetAt(r);
        pNextRec = recs.GetAt(r + 1);
        pCurrValue = static_cast<CTDNumericValue*> (pCurrRec->getValue(attribIdx));
        pNextValue = static_cast<CTDNumericValue*> (pNextRec->getValue(attribIdx));
        if (!pCurrValue || !pNextValue) {
            ASSERT(false);
            return false;
		}

        // Create the fist range if the first value is not equal to the lowest possible value of the concept.
		// m_lowerbound is inclusive and m_upperbound is exclusive for a range. 
		if (r == 0 && pCurrValue->getRawValue()> pContConcept->m_lowerBound){
			cRanges.Add(new CTDRange(pCurrValue->getRawValue(), pContConcept->m_lowerBound));  
			ranges.Add(pCurrValue->getRawValue() - pContConcept->m_lowerBound);
			weights.Add(0.0f);
		}
		
		// Get the class concept
        pClassConcept = pCurrRec->getValue(classIdx)->getCurrentConcept(); 

        // Compute support counters         
        ++((*m_pSplitSupMatrix)[0][pClassConcept->m_childIdx]);	
        --((*m_pSplitSupMatrix)[1][pClassConcept->m_childIdx]);												
        
        // Compute support sums, but class sums remain unchanged
        ++(m_splitSupSums[0]);
        --(m_splitSupSums[1]);

        // Compare with next value. If different, then compute score
        if (pCurrValue->getRawValue() != pNextValue->getRawValue()) {
            if (!computeMaxHelper(m_splitSupSums, m_splitClassSums, *m_pSplitSupMatrix, max))
                return false;

			if (!computeInfoGainHelper(computeEntropy(&m_splitClassSums), m_splitSupSums, m_splitClassSums, *m_pSplitSupMatrix, infoGain))
                return false;

			if (!computeDiscernHelper(m_splitSupSums, discern))
				return false;

			if (!computeNCPSplitHelper(m_splitSupSums, ncp, pCurrValue, pNextValue, pContConcept))	// Compute ncp of the midpoint.
				return false;
			
			cRanges.Add(new CTDRange(pNextValue->getRawValue(), pCurrValue->getRawValue()));  
			ranges.Add(pNextValue->getRawValue()- pCurrValue->getRawValue());
		
#if defined(_TD_SCORE_FUNTION_MAX) 
			weights.Add(max);
#endif

#if defined(_TD_SCORE_FUNCTION_INFOGAIN) 
			weights.Add(infoGain);
#endif

#if defined(_TD_SCORE_FUNTION_DISCERNIBILITY)
			long double A = TD_DISCERN_UPPER_BOUND * 1.0;	
			long double B = TD_DISCERN_LOWER_BOUND * 1.0;
			long long   z = (discern - A) * (TD_NORM_UPPER_BOUND - TD_NORM_LOWER_BOUND);
			long double norm_discern = TD_NORM_LOWER_BOUND + z / (B - A);

			weights.Add(norm_discern);
#endif

#if defined(_TD_SCORE_FUNCTION_NCP)				
			// We want to favor lower values
			float normNCP = (ncp * -1) + getnTrainingRecs();
			weights.Add(normNCP);
#endif

            FLAG = true;
        }
    }

	if (FLAG){
        // srand( (unsigned)time( NULL ) );
	    idx = expoMechSplit(epsilon, &weights, &ranges); 

#if defined(_TD_SCORE_FUNCTION_NCP)
		// Not all values have the same ncp within the same interval.
		// We estimate the ncp of the interval by being the ncp of the midpoint.
		m_splitPoint = (cRanges.GetAt(idx)->m_upperValue + cRanges.GetAt(idx)->m_lowerValue) / 2;
#else
		// Randomly pick a value from the range of the selected interval, since all the values in the interval have the same score.
	    m_splitPoint = (float) (rand() % (int)(cRanges.GetAt(idx)->m_upperValue - cRanges.GetAt(idx)->m_lowerValue + 1) + cRanges.GetAt(idx)->m_lowerValue); 

#endif
	}
	else {
		// All the raw vlaues are same. 
		// m_splitPoint is chosen uniformly from the domian or midpoint (for NCP).
#if defined(_TD_SCORE_FUNCTION_NCP)
		m_splitPoint = (pContConcept->m_upperBound + pContConcept->m_lowerBound) / 2;
#else
		m_splitPoint = (float) (rand() % (int)(pContConcept->m_upperBound - pContConcept->m_lowerBound + 1) + pContConcept->m_lowerBound); 
#endif
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartAttrib::initSplitMatrix(int nConcepts, int nClasses)
{
    // Allocate the matrix
    int dims[] = {nConcepts, nClasses};
    m_pSplitSupMatrix = new CTDMDIntArray(sizeof(dims) / sizeof(int), dims);
    if (!m_pSplitSupMatrix) {
        ASSERT(false);
        return false;
    }

    // Allocate support sums
    m_splitSupSums.SetSize(nConcepts);

    // Allocate class sums
    m_splitClassSums.SetSize(nClasses);

    for (int i = 0; i < nConcepts; ++i) {
        for (int j = 0; j < nClasses; ++j) {
            (*m_pSplitSupMatrix)[i][j] = 0;
            m_splitClassSums.SetAt(j, 0);
        }
        m_splitSupSums.SetAt(i, 0);
    }
    return true;
}



//*****************
// CTDPartAttribs *
//*****************
CTDPartAttribs::CTDPartAttribs()
{
}

CTDPartAttribs::~CTDPartAttribs()
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

//**************
// CTDRanges *
//**************
CTDRanges::CTDRanges()
{
}

CTDRanges::~CTDRanges()
{
}

void CTDRanges::cleanup()
{
    for (int i = 0; i < GetSize(); ++i)
        delete GetAt(i);

    RemoveAll();
}

//**************
// CTDRange *
//**************
CTDRange::CTDRange(float upperValue, float lowerValue)
      : m_upperValue(upperValue), 
        m_lowerValue(lowerValue)
{
}

CTDRange::~CTDRange()
{
}

