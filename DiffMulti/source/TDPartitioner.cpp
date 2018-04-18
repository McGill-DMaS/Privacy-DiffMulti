// TDPartitioner.cpp: implementation of the CTDPartitioner class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include <map>

#if !defined(TDPARTITIONER_H)
    #include "TDPartitioner.h"
#endif


static int gPartitionIndex = 0;
static int gTestPartitionIndex = 0;
static int q = 0;



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTDPartitioner::CTDPartitioner(int nSpecialization, double pBudget, int nTraining) 
    : m_pAttribMgr(NULL),
	  m_pDataMgr(NULL), 
	  m_nSpecialization(nSpecialization), 
	  m_nMaxLevel(0),
      m_pBudget(pBudget),
	  m_nTraining(nTraining),
	  m_remainder(0.0f),
	  m_workingBudget(-1)
{
}

CTDPartitioner::~CTDPartitioner() 
{
	m_tempPartitions.cleanup();
    m_leafPartitions.cleanup();
	m_testLeafPartitions.cleanup();
	m_testTempPartitions.cleanup();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDPartitioner::initialize(CTDAttribMgr* pAttribMgr, CTDDataMgr* pDataMgr)
{
    if (!pAttribMgr || !pDataMgr) {
        ASSERT(false);
        return false;
    }
    m_pAttribMgr = pAttribMgr;
    m_pDataMgr = pDataMgr;    
    return true;
}

//---------------------------------------------------------------------------
// The main algorithm.
//---------------------------------------------------------------------------
bool CTDPartitioner::transformData()
{
    cout << _T("Partitioning data...") << endl << endl;

    // Initialize the first partition.
	// Training records.
    CTDPartition* pRootPartition = initRootPartition();
    if (!pRootPartition)
        return false;

	// Initialize the generalized records of the first partition.
	if(!pRootPartition->initGenRecords(m_pAttribMgr->getAttributes())){
		delete pRootPartition;
		return false;
	}
	
	// We maintain a separate tree structure for test data to perform the same genearalization.
	// Testing records.
	CTDPartition* pTestRootPartition = initTestRootPartition();
    if (!pTestRootPartition)
        return false;

	if(!initializeBudget()){
		ASSERT(false);
		return false;
	}
	
	// Budget used n times, n is the number of continuous attributes.
	pRootPartition->m_nBudgetCount += m_pAttribMgr->getNumConAttribs();

	// Construct raw counts of the partition.
    if (!pRootPartition->constructSupportMatrix(m_workingBudget)) {
        delete pRootPartition;
        return false;
    }
	
    // Compute a score (e.g. Max) for each concept in the current partition.
    if (!pRootPartition->computeScore()) {
        delete pRootPartition;
        return false;
    }

    // Add root partition to m_tempPartitions.
	m_tempPartitions.cleanup();
    m_leafPartitions.cleanup();
	pRootPartition->m_leafPos = m_tempPartitions.AddHead(pRootPartition);
    
	
	// Add testRoot partition to m_testTempPartitions.
	m_testLeafPartitions.cleanup();
	m_testTempPartitions.cleanup();
	pTestRootPartition->m_leafPos = m_testTempPartitions.AddHead(pTestRootPartition);
    

	// Recursively perform m_nSpecialization specializations, depth-first.
	if (!specializePartition(pRootPartition, pTestRootPartition, m_nSpecialization, m_remainder, m_nTraining)) {
		m_tempPartitions.cleanup();
		m_leafPartitions.cleanup();	
		m_testTempPartitions.cleanup();
		m_testLeafPartitions.cleanup();
		return false;
	}

	 // Delete empty test leaf partitions.
    m_testLeafPartitions.deleteEmptyPartitions();

	// List of temporary partitions should be empty.
	if (!m_tempPartitions.IsEmpty() || !m_testTempPartitions.IsEmpty()) { 
		cout << endl;
		cout << "***" << endl;
		cout << _T("CTDPartitioner::transformData(): Warning. List of temp Partitions should be empty.") << endl;
		if (!m_tempPartitions.IsEmpty())
			cout << "m_tempPartitions is not empty." << endl;
		if (!m_testTempPartitions.IsEmpty())
			cout << "m_testTempPartitions is not empty." << endl;
		cout << "***";
		cout << endl << endl;
	}

    cout << _T("\nPartitioning data succeeded.") << endl;
	cout << "Number of input specializations m_nSpecialization     : " << m_nSpecialization << endl;
	cout << "Number of actual specializations q                    : " << q << endl << endl;
    
	return true;
}

//---------------------------------------------------------------------------
// m_leafPartitions contains leaf partitions only.
//
// A partition is leaf if:
//					Case 1: No candidate in partition.
//					Case 2: Exculded when handling Case 2 of nSpecializations.
//					Case 3: Consumed all its dedicated nSpecializations (h = 0),
//							or at max allowable level in the tree (m_nMaxLevel).
//
// How nSpecializations is handled:
//					Case 1: No candidate in partition. Add h to next path.
//					Case 2: h/children is < 1. Check comments.
//					Case 3: Ignored.
//					Case 4: h = 0.
//---------------------------------------------------------------------------
bool CTDPartitioner::specializePartition(CTDPartition*& pRootPartition, CTDPartition* pTestRootPartition, int nSpecializations, double& remainder, int nParentRecords)
{
	double totalRemainder = remainder;	

	// Validate parameters
	if(nSpecializations <= 0 || remainder < 0.0) {
		cerr << _T("CTDPartitioner::specializePartition(): Incorrect nSpecializations or remainder.") << endl;
		cerr << _T("nSpecializations: ") << nSpecializations << endl;
		cerr << _T("remainder	: ") << remainder << endl;
		cerr << _T("Epsilon prime may be too small, thus, obtaining large noises is likely to happen.") << endl;
		return false;
	}
	
	CTDAttrib* pSelectedAttrib = NULL;
    CTDConcept* pSelectedConcept = NULL;
	CTDPartAttrib* pSelectedPartAttrib = NULL;

	// Use expoMech() to select a concept from current partition for specialization
	if(!pRootPartition->pickSpecializeConcept(pSelectedAttrib, pSelectedConcept, pSelectedPartAttrib, m_workingBudget))
		return false;

	// nSpecializations: Case 1
	// Leaf partition: Case 1
	// No concept is a candidate
	// If specialization ended before h=0, remaining h is added to next path		<<======== This is a leaf partition.
	if (!pSelectedAttrib || !pSelectedConcept || !pSelectedPartAttrib) {
		remainder += nSpecializations;
		pRootPartition->m_leafPos		= m_leafPartitions.AddTail(m_tempPartitions.RemoveHead());
		pRootPartition->makeMultiDimAttribs();
		pTestRootPartition->m_leafPos	= m_testLeafPartitions.AddTail(m_testTempPartitions.RemoveHead());
		pRootPartition->m_path.Add("None");
		return true;
	}

	// Budget used once for choosing a partAttrib
	pRootPartition->m_nBudgetCount += 1;	

	// Budget used once to find a split point for a newly created 
	// continuous concept in the child partition if the winner is 
	// a continuous attribute.
	if(pSelectedAttrib->isContinuous())
		pRootPartition->m_nBudgetCount += 1;

	// Budget used once in splitPartitions() below to add Laplace noise to the number of records
	pRootPartition->m_nBudgetCount += 1;
		
	// Split the parent partition based on the selected concept
	int nChildPartitions = -1;
	if (!splitPartitions(pRootPartition, pSelectedPartAttrib, pSelectedAttrib, pSelectedConcept, nChildPartitions, nSpecializations, m_workingBudget)) 
        return false;
	
	if (!splitTestPartitions(pTestRootPartition, pSelectedPartAttrib, pSelectedAttrib, pSelectedConcept)) 
        return false;
   
	// Remove parent partition from m_tempPartitions
	m_tempPartitions.RemoveAt(pRootPartition->m_leafPos);
	delete pRootPartition;
	pRootPartition = NULL;

	// Remove parent partition from testTempPartitions.
    m_testTempPartitions.RemoveAt(pTestRootPartition->m_leafPos);
    delete pTestRootPartition;
    pTestRootPartition = NULL;

	// A specialization counter
	++q;
		
	// nSpecializations: Case 3 IGNORED.
	// If Case 2 is true but not all partitions of 
	// the new nChildPartitions (each h = 1) consumed their h.

	for (int i = 0; i < nChildPartitions; ++i)	
	{
		// Obtain partition from m_tempPartitions.
		pRootPartition = getNextPartition();	
		pTestRootPartition = getNextTestPartition();

		if (!pRootPartition || !pTestRootPartition) {
			cerr << _T("CTDPartitioner::specializePartition(): no next partition.") << endl;
			ASSERT(false);
			return false;
		}

		// Precalculated in splitPartitions()
		int nLocalSpecializations = pRootPartition->m_nLocalSpecializations;
		totalRemainder = 0.0;

		// Noise is too large and made nSpecializations of this partition below zero.
		if (nLocalSpecializations < 0)
			nLocalSpecializations = 0;

		// nSpecializations: Case 4:
		// Leaf partition: Case 3.													<<========= This is a leaf partition.
		if ((nLocalSpecializations == 0) || (pRootPartition->m_nLevelCount >= m_nMaxLevel)) {     
			pRootPartition->m_leafPos		= m_leafPartitions.AddTail(m_tempPartitions.RemoveHead());
			pRootPartition->makeMultiDimAttribs();
			pTestRootPartition->m_leafPos	= m_testLeafPartitions.AddTail(m_testTempPartitions.RemoveHead());
			pRootPartition->m_path.Add("None");
			continue;
		}


		// If nSpecializations >= 1.
		// Depth-first: compute score of each m_partAttrib in the next partition.
		if (!pRootPartition->computeScore()) 
			return false; 

		if (!specializePartition(pRootPartition, pTestRootPartition, nLocalSpecializations, totalRemainder, pRootPartition->getNumRecords())) {
			cerr << _T("CTDPartitioner: Cannot specialize on partition.") << endl;
			ASSERT(false);
			return NULL;
		}
	}

	remainder = totalRemainder;

	return true;
	
}

//---------------------------------------------------------------------------
// Deapth-first
// All partitions are kept in m_tempPartitions
//---------------------------------------------------------------------------
CTDPartition* CTDPartitioner::getNextPartition()
{
	CTDPartitions* pPartitions = NULL;

	// Size should be >= 1
	if (m_tempPartitions.GetSize() < 1) {
		cerr << _T("CTDPartitioner::getNextPartition(): Empty m_leafPartitions.") << endl;
		return NULL;
	}

	return m_tempPartitions.GetHead();
}

//---------------------------------------------------------------------------
// Deapth-first
// All test partitions are kept in m_testTempPartitions
//---------------------------------------------------------------------------
CTDPartition* CTDPartitioner::getNextTestPartition()
{
	CTDPartitions* pPartitions = NULL;

	// Size should be >= 1
	if (m_testTempPartitions.GetSize() < 1) {
		cerr << _T("CTDPartitioner::getNextPartition(): Empty m_testTempPartitions.") << endl;
		return NULL;
	}

	return m_testTempPartitions.GetHead();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

bool CTDPartitioner::addNoise()
{
	CTDPartition* pLeafPartition = NULL;
	int budgetUsage = 0;
	int maxBudgetUsage = 0;
	
	// Get max budget usage
	for (POSITION childPos = m_leafPartitions.GetHeadPosition(); childPos != NULL;) {
        pLeafPartition = m_leafPartitions.GetNext(childPos);
		budgetUsage = pLeafPartition->m_nBudgetCount;

		if (budgetUsage > maxBudgetUsage)
			maxBudgetUsage = budgetUsage;
    }

	// Remaining budget is >= (original value / 2)
	m_pBudget = m_pBudget - (maxBudgetUsage * m_workingBudget);

	for (POSITION leafPos = m_leafPartitions.GetHeadPosition(); leafPos != NULL;) {
        pLeafPartition = m_leafPartitions.GetNext(leafPos);

        // Add noise to each leaf partition
		// Parallel composition: each leaf partition gets the same budget
		if (!pLeafPartition->addNoise(m_pBudget)) {
            ASSERT(false);
            return false;
        }
    }
	
	cout << _T("The number of leaf partitions is ")<< m_leafPartitions.GetSize()<< endl;
	cout << _T("Remaining privacy budget for leaf nodes: ")<< m_pBudget << endl;
	cout << _T("Unused nSpecializations: ") << m_nSpecialization - q + m_remainder << endl << endl;

	return true;
}


//---------------------------------------------------------------------------
// Build a partition with attributes and raw training records.
//---------------------------------------------------------------------------
CTDPartition* CTDPartitioner::initRootPartition()
{
    CTDPartition* pPartition = new CTDPartition(gPartitionIndex++, m_pAttribMgr->getAttributes());
    if (!pPartition)
        return NULL;

    CTDRecords* pRecs = m_pDataMgr->getRecords();	//** Gets training records.
    if (!pRecs) {
        delete pPartition;
        return NULL;
    }

    int nRecs = pRecs->GetSize();
    for (int i = 0; i < nRecs; ++i) {
        if (!pPartition->addRecord(pRecs->GetAt(i))) {
            delete pPartition;
            return NULL;
        }
    }

    if (pPartition->getNumRecords() <= 0) {
        cerr << _T("CTDPartitioner: Zero number of records in root partition.") << endl;
        delete pPartition;
        ASSERT(false);
        return NULL;
    }
  
  return pPartition;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
CTDPartition* CTDPartitioner::initTestRootPartition()
{
    CTDPartition* pPartition = new CTDPartition(gTestPartitionIndex++, m_pAttribMgr->getAttributes());
    if (!pPartition)
        return NULL;

    CTDRecords* pRecs = m_pDataMgr->getTestRecords();
    if (!pRecs) {
        delete pPartition;
        return NULL;
    }

    int nRecs = pRecs->GetSize();
    for (int i = 0; i < nRecs; ++i) {
        if (!pPartition->addRecord(pRecs->GetAt(i))) {
            delete pPartition;
            return NULL;
        }
    }

    if (pPartition->getNumRecords() <= 0) {
        cerr << _T("CTDPartitioner: Zero number of records in root test partition.") << endl;
        delete pPartition;
        ASSERT(false);
        return NULL;
    }
 
  return pPartition;
}

//---------------------------------------------------------------------------
// Compute epsilon prime based on an estimation of the longest root-to-leaf path, m_nMaxLevel.
// m_nMaxLevel is the level sum of all hierarchies.
// A continuous attributes is assumed to have TD_CONT_ATTR_LEVELS levels. 
//---------------------------------------------------------------------------
bool CTDPartitioner::initializeBudget()
{
	ASSERT(m_pBudget > 0 && m_nSpecialization > 0);

	m_nMaxLevel = 0;
	for (int i = 0; i < m_pAttribMgr->getAttributes()->GetSize() - 1; ++i)
		m_nMaxLevel += m_pAttribMgr->getAttribute(i)->getMaxDepth(); 

	m_workingBudget = m_pBudget/(2 * (m_pAttribMgr->getNumConAttribs() + 3 * m_nMaxLevel));		

	return true;
}	

//---------------------------------------------------------------------------
// Distribute records from parent paritition to child partitions.
// Update m_tempPartitions.
// Compute statistics for the new partitions.
//---------------------------------------------------------------------------
bool CTDPartitioner::splitPartitions(CTDPartition* pParentPartition, 
									 CTDPartAttrib* pSplitPartAttrib, 
									 CTDAttrib* pSplitAttrib, 
									 CTDConcept* pSplitConcept, 
									 int& nChildPartitions,
									 const int nSpecializations,
									 double epsilon)
{
    ASSERT(pParentPartition && pSplitPartAttrib && pSplitAttrib && pSplitConcept);

    CTDPartitions childPartitions;
    CTDPartition* pChildPartition = NULL;
    
#ifdef _DEBUG_PRT_INFO                        
        cout << _T("----------------------[Splitting Parent Partition]------------------------") << endl;
        cout << *pParentPartition;
#endif
		
	// Distribute records from parent paritition to child partitions
	if (!distributeRecords(pParentPartition, pSplitPartAttrib, pSplitAttrib, pSplitConcept, childPartitions)) 
        return false;
	
	nChildPartitions = childPartitions.GetSize();
	int nSpecSum = 0;

	// Generate noise for every child partition.
	int i = 0;
	CTDIntArray noises;
	int noiseSum = 0;
	noises.SetSize(nChildPartitions);
	for (i = 0; i < nChildPartitions; ++i) {
		noises[i] = (int)laplaceNoise(epsilon);
		// Keep positive noise
		if (noises[i] < 0)
			noises[i] = 0;
		noiseSum += noises[i];
	}

	i = 0;
	int x = -1;
	double denominator = 0;
	for (POSITION childPos = childPartitions.GetTailPosition(); childPos != NULL; ++i) {
		pChildPartition = childPartitions.GetPrev(childPos);

		// Fair distribution of nSpecializations 
		// Laplace noise is added to a record number to make the distribution Diff private
		// If count is too small, multiply by a large constant then add noise (data independent)

		if (pParentPartition->getNumRecords() != 0) {
			denominator = (long long(pParentPartition->getNumRecords()) * long long(TD_CONST)) + noiseSum;
			if (denominator == 0) {
				cerr << "CTDPartitioner::splitPartitions: division by zero. You may consider: " << endl;
				cerr << "Increasing TC_CONST to minimize the effect of laplace noise on the number of records when distributing nSpec. " << endl;
				ASSERT(false);
				return false;
			}
			
			pChildPartition->m_nLocalSpecializations = (double((long long(pChildPartition->getNumRecords()) * long long(TD_CONST)) + noises[i]) / double((long long(pParentPartition->getNumRecords()) * long long(TD_CONST)) + noiseSum)) * (nSpecializations - 1);
			x = (double(long long(pChildPartition->getNumRecords()) * long long(TD_CONST)) / double(long long(pParentPartition->getNumRecords()) * long long(TD_CONST))) * (nSpecializations - 1);
		}
		
		else {
			pChildPartition->m_nLocalSpecializations = 0; x = 0; }

#ifdef _DEBUG_PRT_INFO
		cout << "noise								: " << noises[i] << endl;
		cout << "nSpecializations - 1				: " << nSpecializations - 1 << endl;
		cout << "pChildPartition->getNumRecords()	: " << pChildPartition->getNumRecords() << endl;
		cout << "pParentPartition->getNumRecords()	: " << pParentPartition->getNumRecords() << endl;
		cout << "pChildPartition->m_nLocalSpecializations before noise: " << x << endl;
		cout << "pChildPartition->m_nLocalSpecializations after noise : " << pChildPartition->m_nLocalSpecializations << endl;
		cout << "Difference                                           : " << pChildPartition->m_nLocalSpecializations - x << endl << endl;
#endif

		nSpecSum += pChildPartition->m_nLocalSpecializations;

		// Add child partitions to m_tempPartitions.
		// First-Input-Last-Output: child partitions added to the head of m_tempPartitions.
		pChildPartition->m_leafPos = m_tempPartitions.AddHead(pChildPartition);

#ifdef _DEBUG_PRT_INFO
            cout << _T("------------------------[Splitted Child Partition]------------------------") << endl;
            cout << *pChildPartition;
#endif
		// Compute support matrix
		// Construct raw counts
		if (!pChildPartition->constructSupportMatrix(m_workingBudget)) {
			ASSERT(false);
			return false;
		}
	}

	// Check if all nSpecializations has been distributed
	// Data independent

	// Case 1: Not distributed entirely
	// Evenly distribute the remaining
	if (nSpecializations - 1 > nSpecSum) {
		int extra = nSpecializations - 1 - nSpecSum;
#ifdef _DEBUG_PRT_INFO
		cout << "\n";
		cout << "Parent nSpec - 1     : " << nSpecializations - 1 << endl;
		cout << "Num of child Partitions : " << nChildPartitions << endl;
		cout << "extra = nSpecializations - 1 - nSpecSum = " << nSpecializations - 1 - nSpecSum << endl << endl;
#endif
		for (POSITION childPos = childPartitions.GetHeadPosition(); childPos != NULL;) {
			pChildPartition = childPartitions.GetNext(childPos);
			pChildPartition->m_nLocalSpecializations += 1;
#ifdef _DEBUG_PRT_INFO
			cout << "Adding to pChildPartition->m_nLocalSpecializations: " << pChildPartition->m_nLocalSpecializations << endl << endl;
#endif
			extra--;
			if (extra == 0)
				break;
			else if (childPos == NULL && extra != 0)
				childPos = childPartitions.GetHeadPosition();
			else
				;
		}
	}
	// Case 2: Overly distributed
	// Evenly reduce the extra
	else if (nSpecSum > nSpecializations - 1) {
		int extra = nSpecializations - 1 - nSpecSum;
#ifdef _DEBUG_PRT_INFO
		cout << "\n";
		cout << "Parent nSpec - 1     : " << nSpecializations - 1 << endl;
		cout << "Num of child Partitions : " << nChildPartitions << endl;
		cout << "extra = nSpecializations - 1 - nSpecSum = " << nSpecializations - 1 - nSpecSum << endl << endl;
#endif
		for (POSITION childPos = childPartitions.GetHeadPosition(); childPos != NULL;) {
			pChildPartition = childPartitions.GetNext(childPos);
			// m_nLocalSpecializations should be >= 0.
			if (pChildPartition->m_nLocalSpecializations <= 0)
				continue;
			pChildPartition->m_nLocalSpecializations -= 1;
#ifdef _DEBUG_PRT_INFO
			cout << "Subtracting from pChildPartition->m_nLocalSpecializations: " << pChildPartition->m_nLocalSpecializations << endl << endl;
#endif
			extra++;
			if (extra == 0)
				break;
			else if (childPos == NULL && extra != 0)
				childPos = childPartitions.GetHeadPosition();
			else
				;
		}
	}
	else
		;

    return true;
}

//---------------------------------------------------------------------------
// Spliting records among test partitions like the previous function
//---------------------------------------------------------------------------
bool CTDPartitioner::splitTestPartitions(CTDPartition* pParentPartition,
										 CTDPartAttrib* pSelectedPartAttrib,
										 CTDAttrib* pSplitAttrib,
										 CTDConcept* pSplitConcept)
{
	ASSERT (pParentPartition && pSelectedPartAttrib && pSplitAttrib && pSplitConcept);

    CTDPartitions childPartitions;
    CTDPartition* pChildPartition = NULL;

#ifdef _DEBUG_PRT_INFO                        
        cout << _T("--------------------[Splitting Parent Test Partition]---------------------") << endl;
        cout << *pParentPartition;
#endif
        
    // Distribute records from parent paritition to child partitions.
    if (!testDistributeRecords(pParentPartition, pSelectedPartAttrib, pSplitAttrib, pSplitConcept, childPartitions))
        return false;

	for (POSITION childPos = childPartitions.GetTailPosition(); childPos != NULL;) {
		pChildPartition = childPartitions.GetPrev(childPos);
		
		// Add child partitions to m_testTempPartitions.
		// First-Input-Last-Output: child partitions added to the head of m_testTempParitions.
		pChildPartition->m_leafPos = m_testTempPartitions.AddHead(pChildPartition);

		#ifdef _DEBUG_PRT_INFO
            cout << _T("---------------------[Splitted Child Test Partition]----------------------") << endl;
            cout << *pChildPartition;
		#endif
	}

    return true;
}


//---------------------------------------------------------------------------
// Distribute records from parent paritition to child partitions.
//---------------------------------------------------------------------------
bool CTDPartitioner::distributeRecords(CTDPartition*  pParentPartition,
									   CTDPartAttrib* pSplitPartAttrib,
                                       CTDAttrib*     pSplitAttrib, 
                                       CTDConcept*    pSplitConcept,
                                       CTDPartitions& childPartitions) 
{
    childPartitions.RemoveAll();

	// Construct a partition for each child concept. 
	if (pSplitAttrib->isContinuous()) {
		CTDPartition* pPartition1 = new CTDPartition(gPartitionIndex++, m_pAttribMgr->getAttributes(), pParentPartition, pSplitAttrib->m_attribIdx);
		pPartition1->m_leafPos = childPartitions.AddTail(pPartition1);

		CTDPartition* pPartition2 = new CTDPartition(gPartitionIndex++, m_pAttribMgr->getAttributes(), pParentPartition, pSplitAttrib->m_attribIdx);
		pPartition2->m_leafPos = childPartitions.AddTail(pPartition2);
	}
	else {
		for (int childIdx = 0; childIdx < pSplitConcept->getNumChildConcepts(); ++childIdx)	{
			CTDPartition* pPartition = new CTDPartition(gPartitionIndex++, m_pAttribMgr->getAttributes(), pParentPartition, pSplitAttrib->m_attribIdx);
			pPartition->m_leafPos = childPartitions.AddTail(pPartition);
		}
	}

    // Generate the generalized records for every child partition.
	CTDPartition* pChildPartition = NULL;
	int idx = 0;
	for (POSITION childPos = childPartitions.GetHeadPosition(); childPos != NULL; ++idx) {
        pChildPartition = childPartitions.GetNext(childPos);

		pChildPartition->m_path.Add(pSplitAttrib->m_attribName);

		if (!pChildPartition->genRecords(pParentPartition, pSplitAttrib, pSplitConcept, m_pAttribMgr->getAttributes(), idx)) {
            ASSERT(false);
            return false;
        }
    }

    // Scan through each record in the parent partition and
    // add records to the corresponding child partition based
    // on the child concept
    CTDRecord* pRec = NULL;
    CTDValue* pSplitValue = NULL;
    POSITION childPartitionPos = NULL;
    int childConceptIdx = -1;
    int splitIdx = pSplitAttrib->m_attribIdx;
    int nRecs = pParentPartition->getNumRecords();
    //ASSERT(nRecs > 0);
    for (int r = 0; r < nRecs; ++r) {
        pRec = pParentPartition->getRecord(r);
        pSplitValue = pRec->getValue(splitIdx);

        // Lower the concept by one level
		if (!pSplitValue->lowerCurrentConcept(pSplitPartAttrib)) {
            cerr << _T("CTDPartitioner: Should not specialize on this concept.");
            childPartitions.cleanup();
            ASSERT(false);
            return false;
        }

        // Get the child concept of the current concept in this record
        childConceptIdx = pSplitValue->getCurrentConcept()->m_childIdx;
        ASSERT(childConceptIdx != -1);
        childPartitionPos = childPartitions.FindIndex(childConceptIdx);
        ASSERT(childPartitionPos);

        // Add the record to this child partition
        if (!childPartitions.GetAt(childPartitionPos)->addRecord(pRec)) {
            childPartitions.cleanup();
            ASSERT(false);                
            return false;
        }
    }

	// Empty partitions are not deleted because we release noisy counts: 0 +- noise.

    return true;
}

//---------------------------------------------------------------------------
// Distribute records from parent paritition to child partitions
//---------------------------------------------------------------------------
bool CTDPartitioner::testDistributeRecords(	CTDPartition*  pParentPartition,
											CTDPartAttrib* pSplitPartAttrib,
											CTDAttrib*     pSplitAttrib, 
											CTDConcept*    pSplitConcept, 
											CTDPartitions& childPartitions) 
{
    childPartitions.RemoveAll();
	int x = -1;

	// Construct a partition for each child concept
	if (pSplitAttrib->isContinuous()) {
		CTDPartition* pPartition1 = new CTDPartition(gTestPartitionIndex++, m_pAttribMgr->getAttributes(), pParentPartition);
		pPartition1->m_leafPos = childPartitions.AddTail(pPartition1);
		CTDPartition* pPartition2 = new CTDPartition(gTestPartitionIndex++, m_pAttribMgr->getAttributes(), pParentPartition);
		pPartition2->m_leafPos = childPartitions.AddTail(pPartition2);
	}
	else {
		for (int childIdx = 0; childIdx < pSplitConcept->getNumChildConcepts(); ++childIdx)	{
			CTDPartition* pPartition = new CTDPartition(gTestPartitionIndex++, m_pAttribMgr->getAttributes(), pParentPartition);
			pPartition->m_leafPos = childPartitions.AddTail(pPartition);
		}
	}

    

    // Scan through each record in the parent partition and
    // add records to the corresponding child partition based
    // on the child concept
    CTDRecord* pRec = NULL;
    CTDValue* pSplitValue = NULL;
    POSITION childPartitionPos = NULL;
    int childConceptIdx = -1;
    int splitIdx = pSplitAttrib->m_attribIdx;
    int nRecs = pParentPartition->getNumRecords();
    //ASSERT(nRecs > 0);
    for (int r = 0; r < nRecs; ++r) {
        pRec = pParentPartition->getRecord(r);
        pSplitValue = pRec->getValue(splitIdx);

        // Lower the concept by one level
        if (!pSplitValue->lowerCurrentConcept(pSplitPartAttrib)) {
            cerr << _T("CTDPartition: Should not specialize on this concept.");
            childPartitions.cleanup();
            ASSERT(false);
            return false;
        }

        // Get the child concept of the current concept in this record
        childConceptIdx = pSplitValue->getCurrentConcept()->m_childIdx;
        ASSERT(childConceptIdx != -1);
        childPartitionPos = childPartitions.FindIndex(childConceptIdx);
        ASSERT(childPartitionPos);

        // Add the record to this child partition
        if (!childPartitions.GetAt(childPartitionPos)->addRecord(pRec)) {
            childPartitions.cleanup();
            ASSERT(false);                
            return false;
        }
    }

	// Empty partitions are deleted at the end
    return true;
}
