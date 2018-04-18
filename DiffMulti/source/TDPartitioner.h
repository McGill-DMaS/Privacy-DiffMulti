// TDPartitioner.h: interface for the CTDPartitioner class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDPARTITIONER_H)
#define TDPARTITIONER_H

#if !defined(TDATTRIBMGR_H)
    #include "TDAttribMgr.h"
#endif

#if !defined(TDDATAMGR_H)
    #include "TDDataMgr.h"
#endif

#if !defined(TDPARTITION_H)
    #include "TDPartition.h"
#endif

class CTDPartitioner  
{
public:
    CTDPartitioner(int nSpecialization,	double privacyB, int nTraining);
    virtual ~CTDPartitioner();

// operations
    bool initialize(CTDAttribMgr* pAttribMgr, CTDDataMgr* pDataMgr);
    bool transformData();
	bool addNoise();
    CTDPartitions* getLeafPartitions() { return &m_leafPartitions; };
	CTDPartitions* getTestLeafPartitions() { return &m_testLeafPartitions; };


protected:
    CTDPartition* initRootPartition();
	CTDPartition* initTestRootPartition();
	bool initializeBudget();
    bool splitPartitions(CTDPartition*	pParentPartition,
						 CTDPartAttrib* pSelectedPartAttrib, 
						 CTDAttrib*		pSplitAttrib, 
						 CTDConcept*	pSplitConcept, 
						 int&			nChildPartitions,
						 const int		nSpecializations,
						 double			epsilon);

	bool splitTestPartitions(CTDPartition*	pParentPartition,
							 CTDPartAttrib* pSelectedPartAttrib, 
							 CTDAttrib* pSplitAttrib, 
							 CTDConcept* pSplitConcept);

    bool distributeRecords(CTDPartition*  pParentPartition, 
						   CTDPartAttrib* pSplitPartAttrib,
                           CTDAttrib*     pSplitAttrib, 
                           CTDConcept*    pSplitConcept,
                           CTDPartitions& childPartitions);

	bool testDistributeRecords(CTDPartition*  pParentPartition, 
							   CTDPartAttrib* pSplitPartAttrib,
							   CTDAttrib*	  pSplitAttrib, 
							   CTDConcept*    pSplitConcept,
                               CTDPartitions& childPartitions);

	inline float log2f(float x) { return log10f(x) / log10f(2); };
	bool specializePartition(CTDPartition*& pRootPartition, CTDPartition* pTestRootPartition, int nSpecializations, double& remainder, int nParentRecords);
	CTDPartition* getNextPartition();
	CTDPartition* getNextTestPartition();
	void makeMultiDimAttrib(CTDPartition* pPartition);
 

// attributes
    CTDAttribMgr*		m_pAttribMgr;
    CTDDataMgr*			m_pDataMgr;
	CTDPartitions		m_tempPartitions;	// For all partitions.
    CTDPartitions		m_leafPartitions;	// For leaf partitions only. 
	CTDPartitions		m_testTempPartitions;
	CTDPartitions		m_testLeafPartitions;
	int		m_nSpecialization;
	int		m_nMaxLevel;
	int		m_nTraining;
	double	m_remainder;
	double	m_pBudget;
	double	m_workingBudget; 
};

#endif
