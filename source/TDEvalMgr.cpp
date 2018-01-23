// TDAttribMgr.cpp: implementation of the CTDAttribMgr class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDEVALMGR_H)
    #include "TDEvalMgr.h"
#endif

CTDEvalMgr::CTDEvalMgr() 
{
}

CTDEvalMgr::~CTDEvalMgr() 
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDEvalMgr::initialize(CTDAttribMgr* pAttribMgr, CTDPartitioner* pPartitioner)
{
    if (!pAttribMgr || !pPartitioner) {
        ASSERT(false);
        return false;
    }
    m_pAttribMgr = pAttribMgr;
    m_pPartitioner = pPartitioner;
    return true;
}

//---------------------------------------------------------------------------
// Count the number of distortions.
// Each time a record is generalized from a child value to a parent value, 
// we charge 1 unit of distortion.  
//---------------------------------------------------------------------------
bool CTDEvalMgr::countNumDistortions(int& catDistortion, float& contDistortion)
{
    cout << _T("Counting number of distortions...") << endl;
    catDistortion = 0;
    contDistortion = 0.0f;
    int nRecs = 0, nValues = 0;    
    CTDRecord* pRec = NULL;
    CTDAttrib* pAttrib = NULL;
    CTDPartition* pPartition = NULL;
    CTDValue* pValue = NULL;
    CTDConcept* pCurrentConcept = NULL;
    CTDConcept* pRawConcept = NULL;
    CTDPartitions* pLeafPartitions = m_pPartitioner->getLeafPartitions();

    // For each partition
    for (POSITION leafPos = pLeafPartitions->GetHeadPosition(); leafPos != NULL;) {
        pPartition = pLeafPartitions->GetNext(leafPos);
        nRecs = pPartition->getNumRecords();

        // For each record
        for (int r = 0; r < nRecs; ++r) {
            pRec = pPartition->getRecord(r);
            nValues = pRec->getNumValues();

            // For each value
            for (int v = 0; v < nValues; ++v) {
                pAttrib = m_pAttribMgr->getAttribute(v);
                if (!pAttrib->m_bVirtualAttrib)
                    continue;

                pValue = pRec->getValue(v);
                pCurrentConcept = pValue->getCurrentConcept();
                if (pAttrib->isContinuous()) {
                    CTDContConcept* pContConcept = (CTDContConcept*) pCurrentConcept;
                    CTDContConcept* pRoot = (CTDContConcept*) pAttrib->getConceptRoot();
                    contDistortion += (pContConcept->m_upperBound - pContConcept->m_lowerBound) / (pRoot->m_upperBound - pRoot->m_lowerBound);
                }
                else {
                    pRawConcept = ((CTDStringValue*) pValue)->getRawConcept();
#if defined(_TD_SCORE_FUNTION_TRANSACTION)
                    // In case of transaction data, count a distortion only if suppressing "1".
                    if (pRawConcept->m_conceptValue.CompareNoCase(TD_TRANSACTION_ITEM_PRESENT) != 0)
                        continue;
#endif
                    if (pRawConcept->m_depth < 0 || pCurrentConcept->m_depth < 0) {
                        cout << _T("CSAEvalMgr::countNumDistortions: Negative depth.") << endl;
                        ASSERT(false);
                        return false;
                    }
                    catDistortion += pRawConcept->m_depth - pCurrentConcept->m_depth;
                }
            }
        }
    }
    cout << _T("Counting number of distortions succeeded.") << endl;
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDEvalMgr::countNumDiscern(long long& catDiscern)
{
    cout << _T("Counting discernibility...") << endl;
    catDiscern = 0;
    int nNoisyRecs = 0;    
    CTDPartition* pPartition = NULL;
    CTDPartitions* pLeafPartitions = m_pPartitioner->getLeafPartitions();

    // For each partition.
    for (POSITION leafPos = pLeafPartitions->GetHeadPosition(); leafPos != NULL;) {
        pPartition = pLeafPartitions->GetNext(leafPos);
		for (int j = 0; j < pPartition->getNumClasses(); ++j) {
			nNoisyRecs += pPartition->m_classNoisySums[j];
		}
		
        catDiscern += square(nNoisyRecs);
		nNoisyRecs = 0;
    }

    cout << _T("Counting discernibility succeeded.") << endl << endl;
    return true;
}

//---------------------------------------------------------------------------
// Comput the NCP of the output data set. Use the noisy counts in partitions.
// Summary:
//	Each NCP score belongs to a single data value in every raw over all the attributes.
//	The NCP of the entire data set is the average NCP of all data values' NCP scores.
//---------------------------------------------------------------------------
bool CTDEvalMgr::countNumTotalNCP(float& ncp)	
{
	cout << _T("Counting Normalized Certainty Penalty NCP for the entire data set...") << endl;
	ncp = 0.0f;
	float ncpRecSum = 0.0;
	float numerator = 0.0f;	// ncp's of all items in data set.
	int nTotalSupp = 0;	// Number of all items in data set.

	// For each leaf partition
    CTDPartitions* pLeafPartitions = m_pPartitioner->getLeafPartitions();
    for (POSITION leafPos = pLeafPartitions->GetHeadPosition(); leafPos != NULL;) {
        CTDPartition* pPartition = pLeafPartitions->GetNext(leafPos);		
		CTDRecord* pGenRec = pPartition->getGenRecords()->GetAt(0);
		int nValues = pGenRec->getNumValues();

		// For each value
		for (int v = 0; v < nValues - 1; ++v) {
			CTDAttrib* pAttrib = m_pAttribMgr->getAttribute(v);
			CTDValue* pValue = pGenRec->getValue(v);
			CTDConcept* pCurrentConcept = pValue->getCurrentConcept();

			if (!pAttrib->m_bVirtualAttrib)
                continue;
								
			// Numerical value
			if (pAttrib->isContinuous()) {
				float numNCP;
				CTDContConcept* pCurrentNumConcept = (CTDContConcept*) pCurrentConcept;
				CTDContConcept* pNumRootConcept = (CTDContConcept*) pAttrib->getConceptRoot();
				numNCP = (pCurrentNumConcept->m_upperBound - pCurrentNumConcept->m_lowerBound) / (pNumRootConcept->m_upperBound - pNumRootConcept->m_lowerBound);
				ncpRecSum += numNCP;
				continue;
			}

			// Categorical value
			else {
				float discNCP;
				pCurrentConcept->computeNCPHelper(discNCP);
				ncpRecSum += discNCP;
				continue;
			}
		}
		
		// For each general recrod
		for (int j = 0; j < pPartition->getNumClasses(); ++j) {
			numerator = numerator + (ncpRecSum * pPartition->m_classNoisySums[j]);
			nTotalSupp = nTotalSupp + ((nValues - 1) * pPartition->m_classNoisySums[j]);
		}

		ncpRecSum = 0;
    }

	ncp = numerator / nTotalSupp;

	cout << _T("Counting NCP succeeded.") << endl << endl;
	return true;
}

//---------------------------------------------------------------------------
// For categorical attributes only
//---------------------------------------------------------------------------
bool CTDEvalMgr::calPrecision(float& precision)
{
    cout << _T("Calculating precision...") << endl;
    precision = 0.0f;    
    int totalHeight = 0;
    int totalPathLength = 0;

    // For each partition
    CTDPartitions* pLeafPartitions = m_pPartitioner->getLeafPartitions();
    for (POSITION leafPos = pLeafPartitions->GetHeadPosition(); leafPos != NULL;) {
        CTDPartition* pPartition = pLeafPartitions->GetNext(leafPos);

        // For each record
        for (int r = 0; r < pPartition->getNumRecords(); ++r) {
            CTDRecord* pRec = pPartition->getRecord(r);

            // For each value
            for (int v = 0; v < pRec->getNumValues(); ++v) {
                CTDAttrib* pAttrib = m_pAttribMgr->getAttribute(v);
                if (!pAttrib->m_bVirtualAttrib || pAttrib->isContinuous())
                    continue;

                CTDValue* pValue = pRec->getValue(v);
                CTDConcept* pCurrentConcept = pValue->getCurrentConcept();
                CTDConcept* pRawConcept = ((CTDStringValue*) pValue)->getRawConcept();
                if (pRawConcept->m_depth < 0 || pCurrentConcept->m_depth < 0) {
                    cout << _T("CTDEvalMgr::calPrecision: Negative depth.") << endl;
                    ASSERT(false);
                    return false;
                }

                totalHeight += pRawConcept->m_depth - pCurrentConcept->m_depth;
                totalPathLength += pRawConcept->m_depth;
            }
        }
    }
    precision = 1 - ((float) totalHeight / totalPathLength);
    cout << _T("Calculating precision succeeded.") << endl;
    return true;
}