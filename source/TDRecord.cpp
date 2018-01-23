// TDRecord.cpp: implementation of the CTDRecord class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDRECORD_H)
    #include "TDRecord.h"
#endif

//************
// CTDRecord *
//************

CTDRecord::CTDRecord()
    : m_recordID(0)
{
}

CTDRecord::~CTDRecord() 
{
    m_values.cleanup();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDRecord::addValue(CTDValue* pValue)
{
    try {
        m_values.Add(pValue);
        return true;
    }
    catch (CMemoryException&) {
        ASSERT(false);
        return false;
    }
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CString CTDRecord::toString(bool bRawValue) const
{
    CString str;
    CTDValue* pValue = NULL;
    int nValues = m_values.GetSize();
    for (int v = 0; v < nValues; ++v) {
        pValue = m_values.GetAt(v);
		if (bRawValue) 
			str += pValue->toString(true);
		else 
			str += pValue->toString(false);
  
        if (v == nValues - 1)
            str += TD_RAWDATA_TERMINATOR;
        else
            str += TD_RAWDATA_DELIMETER;
    }
    return str;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CString CTDRecord::toVectorStringForVCluster(bool bIncludeClassAttrib, LPCTSTR delimeter) const
{
    CString str;
    CTDConcept* pCurrConcept = NULL;
    int nValues = m_values.GetSize();
    if (!bIncludeClassAttrib)
        --nValues;
    for (int v = 0; v < nValues; ++v) {
        pCurrConcept = m_values.GetAt(v)->getCurrentConcept();

		// If continuous
        if (pCurrConcept->getAttrib()->isContinuous()) {
            // Normalize a continuous value.
            CTDContConcept* pContConcept = (CTDContConcept*) pCurrConcept;
            float normalizedVal = (((CTDNumericValue*) m_values.GetAt(v))->getRawValue() - pContConcept->m_lowerBound) / (pContConcept->m_upperBound - pContConcept->m_lowerBound);
            LPTSTR tempStr = CTDContConcept::FloatToStr(normalizedVal, TD_CONTVALUE_NUMDEC);
            str += CString(tempStr) + delimeter;
            delete [] tempStr;
            tempStr = NULL;
		}
		// If categorical
		else {
			CTDConcepts* pTargetConcepts = pCurrConcept->getAttrib()->getMultiDimConcepts();
			CTDConcept*	pTargetConcept = NULL;
			int nConcepts = pTargetConcepts->GetSize();
			CTDDiscConcept* pCurrDiscConcept = static_cast <CTDDiscConcept*> (m_values.GetAt(v)->getCurrentConcept());				
			for (int w = 0; w < nConcepts; ++w) {
				pTargetConcept = pTargetConcepts->GetAt(w);

				// Target concept is the root
				// Write 1
				if (pTargetConcept->m_depth == 0) {
					str += "1 ";
					continue;
				}

				// Target concept is not current concept and has same or higher depth in hierarchy tree
				// Write 0
				if ((pCurrDiscConcept->m_conceptValue != pTargetConcept->m_conceptValue) && (pCurrDiscConcept->m_depth <= pTargetConcept->m_depth)) {
					str += "0 ";	
					continue;
				}

				// Target concept is current concept
				// Write 1
				if (pCurrDiscConcept->m_conceptValue == pTargetConcept->m_conceptValue) {
					str += "1 ";
					continue;
				}

				// Target concept is an ancestor
				// Write 1
				if (pCurrDiscConcept->isAncestor(pTargetConcept)) {
					str += "1 ";
					continue;
				}

				// Write 0 otherwise 
				str += "0 ";	
				continue;

			} // END FOR  
		} // END ELSE
	}

	return str;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
ostream& operator<<(ostream& os, const CTDRecord& record)
{
#ifdef _DEBUG_PRT_INFO
    os << _T("[") << record.m_recordID << _T("]\t") << record.toString(true);
#endif
    return os;
}

//*************
// CTDRecords *
//*************
CTDRecords::CTDRecords()
{
}

CTDRecords::~CTDRecords()
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CTDRecords::cleanup()
{
    int nRecs = GetSize();
    for (int i = 0; i < nRecs; ++i)
        delete GetAt(i);

    RemoveAll();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDRecords::sortByAttrib(int attribIdx)
{
    bool ret = quickSort(attribIdx, 0, GetSize() - 1);
#if 0 //def _DEBUG_PRT_INFO
    for (int r = 0; r < GetSize(); ++r) {
        cout << _T("RecordID #") << GetAt(r)->getRecordID() << _T(" ") << getNumericValue(r, attribIdx) << endl;
    }
#endif
    return ret;
}

//---------------------------------------------------------------------------
// Quick sort records by attrib index.
//---------------------------------------------------------------------------
bool CTDRecords::quickSort(int attribIdx, int left, int right)
{
    int p = 0;
    float pivot = 0.0f;
    if (findPivot(attribIdx, left, right, pivot)) {
        p = partition(attribIdx, left, right, pivot);
        if (!quickSort(attribIdx, left, p - 1)) {
            ASSERT(false);
            return false;
        }
        if (!quickSort(attribIdx, p, right)) {
            ASSERT(false);
            return false;
        }
    }
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
float CTDRecords::getNumericValue(int recIdx, int attribIdx)
{
    return (static_cast<CTDNumericValue*> (GetAt(recIdx)->getValue(attribIdx)))->getRawValue();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDRecords::findPivot(int attribIdx, int left, int right, float& pivot)
{
    pivot = 0.0f;
    // left value
    float a = getNumericValue(left, attribIdx);
    // middle value
    float b = getNumericValue(left + (right - left) / 2, attribIdx);
    // right value
    float c = getNumericValue(right, attribIdx);
    // order these 3 values
    orderNumbers(a, b, c);

    if (a < b) {
        pivot = b;
        return true;
    }

    if (b < c) {
        pivot = c;
        return true;
    }

    float pValue = 0.0f, leftValue = 0.0f;
    int p = left + 1;
    while (p <= right) {
        pValue = getNumericValue(p, attribIdx);
        leftValue = getNumericValue(left, attribIdx);
        if (pValue != leftValue) {
            if (pValue < leftValue)
                pivot = leftValue;
            else
                pivot = pValue;
            return true;
        }
        ++p;
    }
    return false;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int CTDRecords::partition(int attribIdx, int left, int right, float pivot) 
{
    while (left <= right) {
        while (getNumericValue(left, attribIdx) < pivot)
            ++left;

        while (getNumericValue(right, attribIdx) >= pivot)
            --right;

        if (left < right) {
            swapRecord(left, right);
            ++left;
            --right;
        }
    }
    return left;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CTDRecords::swapRecord(int recIdxA, int recIdxB)
{
    CTDRecord* pTemp = GetAt(recIdxA);
    SetAt(recIdxA, GetAt(recIdxB));
    SetAt(recIdxB, pTemp);
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
ostream& operator<<(ostream& os, const CTDRecords& records)
{
#ifdef _DEBUG_PRT_INFO
    CTDRecord* pRecord = NULL;
    os << _T("# of records = ") << records.GetSize() << endl;
    if (records.GetSize() > 0) {
        os << *(records.GetAt(0)) << endl;
    }
    /*
    for (int r = 0; r < records.GetSize(); ++r) {        
        pRecord = records.GetAt(r);
        os << *pRecord << endl;
    }
    */
#endif
    return os;
}

