// TDPartition.cpp: implementation of the CTDPartition class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDPARTITION_H)
    #include "TDPartition.h"
#endif

//***************
// CTDPartition *
//***************

CTDPartition::CTDPartition(int partitionIdx, CTDAttribs* pAttribs)
	: m_partitionIdx(partitionIdx),
	  m_leafPos(NULL),
	  m_nBudgetCount(0),
	  m_nLevelCount(0),
	  m_nLocalSpecializations(0)
{
    // Add each attribute
    int nAttribs = pAttribs->GetSize();

    for (int i = 0; i < nAttribs - 1; ++i)
        m_partAttribs.AddTail(new CTDPartAttrib(pAttribs->GetAt(i)));

    // Number of classes
    m_nClasses = pAttribs->GetAt(nAttribs - 1)->getConceptRoot()->getNumChildConcepts();
}

CTDPartition::CTDPartition(int partitionIdx, CTDAttribs* pAttribs, CTDPartition* pParentPartition, int const splitIdx)
	: m_partitionIdx(partitionIdx),
	  m_leafPos(NULL)
{
    // Add each attribute
    int nAttribs = pAttribs->GetSize();
    for (int i = 0; i < nAttribs - 1; ++i) 
        m_partAttribs.AddTail(new CTDPartAttrib(pAttribs->GetAt(i)));

    // Number of classes
    m_nClasses = pAttribs->GetAt(nAttribs - 1)->getConceptRoot()->getNumChildConcepts();

	// Budget usage
	m_nBudgetCount = pParentPartition->m_nBudgetCount;

	// Increase tree level by 1.
	m_nLevelCount = pParentPartition->m_nLevelCount + 1;

	// Inherit m_bCandidate from parent partition
	CTDPartAttrib* pThisPartAttrib;
	CTDPartAttrib* pParentPartAttrib;
	CTDPartAttribs* pParentPartAttribs = pParentPartition->getPartAttribs();
	POSITION pos1, pos2;
	for (pos1 = m_partAttribs.GetHeadPosition(), pos2 = pParentPartAttribs->GetHeadPosition(); pos1 != NULL, pos2 != NULL;) {
		pThisPartAttrib = m_partAttribs.GetNext(pos1);
		pParentPartAttrib = pParentPartAttribs->GetNext(pos2);
		pThisPartAttrib->m_bCandidate = pParentPartAttrib->m_bCandidate;

		if (pThisPartAttrib->getActualAttrib()->isContinuous() && pThisPartAttrib->getActualAttrib()->m_attribIdx != splitIdx) {
			pThisPartAttrib->m_splitPoint = pParentPartAttrib->m_splitPoint;
			pThisPartAttrib->m_pLeftChildCon = pParentPartAttrib->m_pLeftChildCon;
			pThisPartAttrib->m_pRightChildCon = pParentPartAttrib->m_pRightChildCon;
		}
	}
	
	for (int j = 0; j < pParentPartition->m_path.GetSize(); j++)
		m_path.Add(pParentPartition->m_path.GetAt(j));
}

CTDPartition::~CTDPartition() 
{
    while (!m_partAttribs.IsEmpty())
        delete m_partAttribs.RemoveHead();
}
//---------------------------------------------------------------------------
// m_genRecords contains one generalized record for every class value
// m_partAttribs and m_partRecords contain the raw data
//---------------------------------------------------------------------------
bool CTDPartition::initGenRecords( CTDAttribs* pAttribs)
{
	m_genRecords.cleanup();
	int nAttribs = pAttribs->GetSize();

	for (int classInd = 0; classInd < m_nClasses; ++classInd){

		CTDAttrib* pAttrib = NULL;
		CTDValue* pNewValue = NULL;
		CTDRecord* pNewRecord = new CTDRecord(); 
		
		for (int attribID = 0; attribID < nAttribs; ++attribID){
	
			pNewValue = NULL;
			pAttrib = pAttribs->GetAt(attribID);
        
			if (pAttrib->isContinuous())
				pNewValue = new CTDNumericValue(-1.0);
			else
	            pNewValue = new CTDStringValue();

			if (!pNewValue) {
				ASSERT(false);
  				return false;
			}
	
			if (attribID == pAttribs->GetSize() - 1) {
				// Class attribute
				if (!pNewValue->assignGenClassValue(pAttrib, classInd))
					return false;
			}
			else {
				// Ordinary attribute
				// Initialize the current concept to the root concept                 
				if (!pNewValue->initConceptToRoot(pAttrib))
					return false;
			}

			// Add the value to the record
			if (!pNewRecord->addValue(pNewValue))
				return false;

		}    

		if (pNewRecord){
			pNewRecord->setRecordID(m_genRecords.Add(pNewRecord));
		}

	}
	if (m_genRecords.GetSize() != m_nClasses) {
            cerr << _T("CTDPartition::initGenRecords: Number of generalized record is not current.") << endl;
            return false;
    }

	return true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartition::genRecords(CTDPartition*  pParentPartition,
                              CTDAttrib*     pSplitAttrib, 
                              CTDConcept*    pSplitConcept,
							  CTDAttribs*    pAttribs,
							  int			 childInd)
{
	m_genRecords.cleanup();
	int nAttribs = pAttribs->GetSize();
	int splitIdx = pSplitAttrib->m_attribIdx;

	for (int classInd = 0; classInd < m_nClasses; ++classInd){

		CTDAttrib* pAttrib = NULL;
		CTDValue* pNewValue = NULL;
		CTDRecord* pNewRecord = new CTDRecord(); 
		
		for (int attribID = 0; attribID < nAttribs; ++attribID){
	
			pNewValue = NULL;
			pAttrib = pAttribs->GetAt(attribID);

			if (pAttrib->isContinuous())
				pNewValue = new CTDNumericValue(-1.0);
			else
	            pNewValue = new CTDStringValue();

			if (!pNewValue) {
				ASSERT(false);
  				return false;
			}

	
			if (attribID == pAttribs->GetSize() - 1) {
				// Class attribute
				if (!pNewValue->assignGenClassValue(pAttrib, classInd))
					return false;
			}
			else if(attribID == splitIdx) {
				//split attribute
				if (!pNewValue->setSplitCurConcept(pSplitConcept, childInd))
					return false;	
			}

			else 
			{	// Other attributes
				// Initialize the current concept to the parent concept
				if (!pNewValue->setCurConcept(pParentPartition->getGenRecords()->GetAt(0)->getValue(attribID)->getCurrentConcept()))
					return false;
			}

			// Add the value to the record
			if (!pNewRecord->addValue(pNewValue))
				return false;
		}    

		if (pNewRecord){
			pNewRecord->setRecordID(m_genRecords.Add(pNewRecord));
		}

	}
	if (m_genRecords.GetSize() != m_nClasses) {
		cerr << _T("CTDPartition::initGenRecords: Number of generalized record is not current.") << endl;
        return false;
    }

	 return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartition::addRecord(CTDRecord* pRecord)
{
    try {
        m_partRecords.Add(pRecord);
        return true;
    }
    catch (CMemoryException&) {
        ASSERT(false);
        return false;
    }
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartition::constructSupportMatrix(double epsilon)
{
    CTDRecord* pFirstRec = NULL;

	if (getNumRecords() == 0) {
		// Get the first record of the generalized records.
		pFirstRec = getGenRecords()->GetAt(0);
	}
	else {
		// Get the first record of the partition.
		pFirstRec = getRecord(0);
		if (!pFirstRec) {
			ASSERT(false);
			return false;
		}
	}

	// Initialize m_pSupportMatrix of every pPartAttrib in this partition to 0
    int a = 0;
	POSITION partAttribPos;
    CTDPartAttrib* pPartAttrib = NULL;
    CTDConcept* pCurrentConcept = NULL;

    for (POSITION pos = m_partAttribs.GetHeadPosition(); pos != NULL; ++a) {
        partAttribPos = pos;
		pPartAttrib = m_partAttribs.GetNext(pos);

		// m_bCandidate is inherited from the parent partition.
		// Otherwise, m_bCandidate is true by default 
		if (!pPartAttrib->m_bCandidate) {
			pCurrentConcept = pFirstRec->getValue(a)->getCurrentConcept();
			if (pCurrentConcept->isContinuous()) {
#ifdef _DEBUG_PRT_INFO
				cout << "PartAttrib	: " << pPartAttrib->m_pActualAttrib->m_attribName << " is !m_bCandidate" << endl;
				CTDContConcept*  pCurrContConcept = static_cast<CTDContConcept*> (pCurrentConcept);
				cout << "Concept	: [" << pCurrContConcept->m_lowerBound << "-" << pCurrContConcept->m_upperBound << "]" << endl << endl;
#endif
				continue;
			}
		}

		// Find the current concept of this partition attribute
        pCurrentConcept = pFirstRec->getValue(a)->getCurrentConcept();

		// Invalid continuous concept for specialization:
		// If current concept is continuous and contains an interval with size <= 1, mark it as invalid.
		if (pCurrentConcept->isContinuous()) {
			CTDContConcept*  pCurrContConcept = static_cast<CTDContConcept*> (pCurrentConcept);
			if (pCurrContConcept->m_upperBound - pCurrContConcept->m_lowerBound <= 1) {
				pCurrentConcept->m_bCutCandidate = false;
				pPartAttrib->m_bCandidate = false;
#ifdef _DEBUG_PRT_INFO
				cout << "==> Current cont concept [" << pCurrContConcept->m_lowerBound << "-" << pCurrContConcept->m_upperBound << "] interval size <= 1." << endl << endl;
#endif
				continue;
			}
		}

        // Need to find a split point
        if (pPartAttrib->getActualAttrib()->m_bVirtualAttrib) {
            if (!pPartAttrib->divideConcept(epsilon, m_nClasses, pCurrentConcept, this)) {	// Will return true if categorical attrib.
                ASSERT(false);																	
                return false;
            }
        }																
								
		if (!hasChildConcepts(pCurrentConcept, pPartAttrib)) {		
			if (pCurrentConcept->isContinuous()) {
#ifdef _DEBUG_PRT_INFO
				CTDContConcept*  pCurrContConcept = static_cast<CTDContConcept*> (pCurrentConcept);
				cout << "==> Current cont concept [" << pCurrContConcept->m_lowerBound << "-" << pCurrContConcept->m_upperBound << "] has no children." << endl << endl;
#endif
				pPartAttrib->m_bCandidate = false;	
			}
			else {
#ifdef _DEBUG_PRT_INFO
				cout << "==> Current cat concept " << pCurrentConcept->m_conceptValue << " has no children." << endl << endl;
#endif
				pCurrentConcept->m_bCutCandidate = false;	// "true" by default.
				pPartAttrib->m_bCandidate = false;
			}
			continue;
		}

        // Construct the support matrix
        if (!pPartAttrib->initSupportMatrix(pCurrentConcept, m_nClasses)) {
            ASSERT(false);
            return false;
        }
	}

	// Initialize the noisy class sum count
	m_classNoisySums.SetSize(m_nClasses);

    for (int j = 0; j < m_nClasses; ++j)
		m_classNoisySums.SetAt(j, 0);


	
    // Compute the support matrix
    CTDConcept* pClassConcept = NULL;
    CTDConcept* pLowerConcept = NULL;    
    CTDMDIntArray* pSupMatrix = NULL;
    CTDIntArray* pSupSums = NULL;
    CTDIntArray* pClassSums = NULL;
    CTDRecord* pRec = NULL;
    int nRecs = getNumRecords();
    int classIdx = m_partAttribs.GetCount();
    for (int r = 0; r < nRecs; ++r) {       
        pRec = getRecord(r);
        // Get the class concept
        pClassConcept = pRec->getValue(classIdx)->getCurrentConcept();  

		++m_classNoisySums[pClassConcept->m_childIdx];
		
        // Compute support counts for each attribute
        int aIdx = 0;
        for (POSITION pos = m_partAttribs.GetHeadPosition(); pos != NULL; ++aIdx) {
            // The partition attribute
            pPartAttrib = m_partAttribs.GetNext(pos);
			
			if (!pPartAttrib->m_bCandidate)
                continue;
		
            // Get the lower concept value
            pLowerConcept = pRec->getValue(aIdx)->getLowerConcept(pPartAttrib);
            if (!pLowerConcept) {
                cerr << _T("No more child concepts. This should not be a candidate.") << endl;
                ASSERT(false);
                return false;
            }

            // Construct the support matrix.
            pSupMatrix = pPartAttrib->getSupportMatrix();
            if (!pSupMatrix) {
                ASSERT(false);
                return false;
            }
            ++((*pSupMatrix)[pLowerConcept->m_childIdx][pClassConcept->m_childIdx]);

            // Compute the support sum of this matrix.
            pSupSums = pPartAttrib->getSupportSums();
            if (!pSupSums) {
                ASSERT(false);
                return false;
            }
            ++((*pSupSums)[pLowerConcept->m_childIdx]);

            // Compute the class sum of this matrix.
            pClassSums = pPartAttrib->getClassSums();
            if (!pClassSums) {
                ASSERT(false);
                return false;
            }
            ++((*pClassSums)[pClassConcept->m_childIdx]);        
        }
    }

    return true;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartition::addNoise(double epsilon)
{
	if (epsilon <= 0) {
		cerr << _T("Not enough budget.") << endl;
        ASSERT(false);
        return false;
    }
	
	for (int j = 0; j < m_nClasses; ++j){
		// Add noise
		m_classNoisySums[j] = m_classNoisySums[j] + (int)laplaceNoise(epsilon);
		
		// Make zero the negative counts
		if (m_classNoisySums[j] < 0)
			m_classNoisySums[j] = 0;
	}
	
    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
ostream& operator<<(ostream& os, const CTDPartition& partition)
{
#ifdef _DEBUG_PRT_INFO
    os << _T("--------------------------------------------------------------------------") << endl;
    os << _T("Partition #") << partition.m_partitionIdx << endl;
    os << partition.m_partRecords;
#endif
    return os;
}

//---------------------------------------------------------------------------
// Iterate through all partAttribs.
// For every partAttrib, compute the score of its concept.
// Score is stored in the partAttrib.
//---------------------------------------------------------------------------
bool CTDPartition::computeScore()
{    
	CTDRecord* pFirstRec = NULL;
	if ( getNumRecords() == 0){
		// Get the first record of the generalized records
		pFirstRec = getGenRecords()->GetAt(0);
	}
	else {
		// Get the first record of the partition
		pFirstRec = getRecord(0);
		if (!pFirstRec) {
			ASSERT(false);
			return false;
		}
	}
	
	int a = 0;
    CTDPartAttrib* pPartAttrib = NULL;
    CTDConcept* pCurrentConcept = NULL;
    for (POSITION pos = m_partAttribs.GetHeadPosition(); pos != NULL; ++a) {
        // Find the current concept of this attribute
        pPartAttrib = m_partAttribs.GetNext(pos);
		pCurrentConcept = pFirstRec->getValue(a)->getCurrentConcept();
		if (!pCurrentConcept) {
			ASSERT(false);
			return false;
		}
		if (!pPartAttrib->computeScore(getNumClasses(), pCurrentConcept))
			return false;
	}
    
    return true;
}

//---------------------------------------------------------------------------
// Pick a concept for specialization from current partition
//---------------------------------------------------------------------------
bool CTDPartition::pickSpecializeConcept(CTDAttrib*& pSelectedAttrib, CTDConcept*& pSelectedConcept, CTDPartAttrib*& pSelectedPartAttrib, double epsilon)
{
	CTDRecord* pFirstRec = NULL;
	if ( getNumRecords() == 0){
		// Get the first record of the generalized records
		pFirstRec = getGenRecords()->GetAt(0);
	}
	else {
		// Get the first record of the partition
		pFirstRec = getRecord(0);
		if (!pFirstRec) {
			ASSERT(false);
			return false;
		}
	}

	pSelectedAttrib = NULL;
    pSelectedConcept = NULL;
	CTDPartAttribs candidates;
	//candidates.cleanup();
	CTDFloatArray weights;
	CTDPosArray positions;
	POSITION currPos = NULL;
	int idx = 0;

    // Minimize the distortion, so specialize even if score = 0
    float maxInfo = -1.0f;
	int a = 0;
    CTDPartAttrib* pPartAttrib = NULL;
    CTDConcept* pCurrentConcept = NULL;
    for (POSITION pos = m_partAttribs.GetHeadPosition(); pos != NULL; ++a) {
        // Find the current concept of this attribute
		currPos = pos;
        pPartAttrib = m_partAttribs.GetNext(pos);
		// Check if QID attribute
		if(!pPartAttrib->getActualAttrib()->m_bVirtualAttrib)
			continue;

		pCurrentConcept = pFirstRec->getValue(a)->getCurrentConcept();
		if (!pCurrentConcept) {
			ASSERT(false);
			return false;
		}

		if (pCurrentConcept->isContinuous()) {
			if (!pPartAttrib->m_bCandidate)
				continue;
		}
		else {
			if (!pCurrentConcept->m_bCutCandidate)
				continue;
		}

		candidates.AddTail(pPartAttrib);
		positions.Add(currPos);
			
#if defined(_TD_SCORE_FUNTION_MAX) 
			weights.Add(pPartAttrib->m_max);
#endif

#if defined(_TD_SCORE_FUNCTION_INFOGAIN) 
			weights.Add(pPartAttrib->m_infoGain);
#endif

#if defined(_TD_SCORE_FUNTION_DISCERNIBILITY)
			long long discern = pPartAttrib->m_discern;
			long double A = TD_DISCERN_UPPER_BOUND * 1.0;	// Supposed to be TD_DISCERN_LOWER_BOUND, but we want to favor lower discern values (lower discern implies better results) so now a large discern value will match to a low normalized value.
			long double B = TD_DISCERN_LOWER_BOUND * 1.0;
			long long   z = (discern - A) * (TD_NORM_UPPER_BOUND - TD_NORM_LOWER_BOUND);
			long double norm_discern = TD_NORM_LOWER_BOUND + z / (B - A);
			/*cout << "Normalized Discern: " << norm_discern << endl << endl;*/

			weights.Add(norm_discern);	// Favors higher normalized discern values (which represent lower discern values)
#endif

#if defined(_TD_SCORE_FUNCTION_NCP)				
			float normNCP = ((pPartAttrib->m_ncp) * -1) + getnTrainingRecs();

	#ifdef _DEBUG_PRT_INFO
			cout << "[" << std::setw(2) <<  std::left << pPartAttrib->getActualAttrib()->m_attribIdx << "] ";
			cout << std::setw(15) <<  std::left << pPartAttrib->getActualAttrib()->m_attribName;
			cout <<  " m_ncp : " ;
			cout << std::fixed << std::setprecision(2) << std::setw(10) << std::left << pPartAttrib->m_ncp;
			cout << "   normNCP : ";
			cout << std::fixed << std::setprecision(2) << std::setw(10) << std::left << normNCP << endl;
	#endif

			weights.Add(normNCP);
#endif

	}
	
	// No concept is a candidate
	if (weights.IsEmpty()) {
#ifdef _DEBUG_PRT_INFO
		cout << endl;
		cout << endl;
		cout << _T("* * * * * No valid split on current partition. * * * * *") << endl;
#endif

		return true;
	}

	// Use exponential mechanism to select the candidate partAttrib
    idx = expoMech(epsilon, &weights);
	pSelectedPartAttrib = candidates.GetAt(positions.GetAt(idx)); 
	pSelectedAttrib = pSelectedPartAttrib->m_pActualAttrib;
	pSelectedConcept = pFirstRec->getValue(pSelectedAttrib->m_attribIdx)->getCurrentConcept();


#ifdef _DEBUG_PRT_INFO
	cout << endl;
    cout << endl;
	cout << _T("* * * * * Selected split on current partition: ") << endl;
    cout << _T("* * * * * [Selected splitting attribute index = ") << pSelectedAttrib->m_attribIdx << _T(", name = ") << pSelectedAttrib->m_attribName << _T("]") << endl;
    cout << _T("* * * * * [Selected splitting concept flatten index = ") << pSelectedConcept->m_flattenIdx << _T(", name = ") << pSelectedConcept->m_conceptValue << _T("]") << endl;
    cout << _T("* * * * * [Selected splitting concept's InfoGain = ") << pSelectedPartAttrib->m_infoGain << _T("]") << endl;
	cout << _T("* * * * * [Selected splitting concept's Max = ") << pSelectedPartAttrib->m_max << _T("]") << endl;
	cout << _T("* * * * * [Selected splitting concept's Discernibility = ") << pSelectedPartAttrib->m_discern << _T("]") << endl;
	cout << _T("* * * * * [Selected splitting concept's NCP = ") << pSelectedPartAttrib->m_ncp << _T("]") << endl;
	cout << endl;
    cout << endl;


#endif
    
    return true;
}

//---------------------------------------------------------------------------
// Concepts in this partition will be written to the .names file.
// These concepts will be distinct attributes with domain values = {0, 1}.
//---------------------------------------------------------------------------
void CTDPartition::makeMultiDimAttribs()
{
	int aIdx = 0;
	CTDPartAttrib* pPartAttrib = NULL;
	CTDRecord* pRec = getGenRecords()->GetAt(0);
    for (POSITION pos = m_partAttribs.GetHeadPosition(); pos != NULL; ++aIdx) {
		pPartAttrib = m_partAttribs.GetNext(pos);
		pRec->getValue(aIdx)->getCurrentConcept()->m_bFileName = true;
	}

	return;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartition::hasChildConcepts(CTDConcept* pCurrConcept, CTDPartAttrib* pPartAttrib)
{
	if (pCurrConcept->isContinuous()) 
		return (pPartAttrib->m_pLeftChildCon && pPartAttrib->m_pRightChildCon);
	else
		return pCurrConcept->getNumChildConcepts() > 0;
}


//****************
// CTDPartitions *
//****************
CTDPartitions::CTDPartitions()
{
}

CTDPartitions::~CTDPartitions()
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void CTDPartitions::cleanup()
{
	while (!IsEmpty())
		delete RemoveHead();
}

//---------------------------------------------------------------------------
// Delete empty partitions.
//---------------------------------------------------------------------------
void CTDPartitions::deleteEmptyPartitions()
{    
    POSITION tempPos = NULL;
    CTDPartition* pPartition = NULL;
    for (POSITION pos = GetHeadPosition(); pos != NULL;) {
        tempPos = pos;
        pPartition = GetNext(pos);
        if (pPartition->getNumRecords() <= 0) {
            RemoveAt(tempPos);
            delete pPartition;
            pPartition = NULL;
        }
    }
}


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
ostream& operator<<(ostream& os, const CTDPartitions& partitions)
{
#ifdef _DEBUG_PRT_INFO
    CTDPartition* pPartition = NULL;
    for (POSITION pos = partitions.GetHeadPosition(); pos != NULL;) {
        pPartition = partitions.GetNext(pos);
        os << *pPartition << endl;
    }
#endif
    return os;
}

