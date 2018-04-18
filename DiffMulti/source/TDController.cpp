// TDController.cpp: implementation of the TDController class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDCONTROLLER_H)
    #include "TDController.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTDController::CTDController(LPCTSTR rawDataFile, 
                             LPCTSTR attributesFile,
                             LPCTSTR nameFile,
                             LPCTSTR transformedDataFile, 
                             LPCTSTR transformedTestFile, 
                             int nSpecialization,
							 double pBudget,
                             int  nInputRecs,
                             int  nTraining)
    : m_attribMgr(attributesFile, nameFile), 
      m_dataMgr(rawDataFile, transformedDataFile, transformedTestFile, nInputRecs, nTraining),
	  m_partitioner(nSpecialization, pBudget, nTraining)
{
    if (!m_dataMgr.initialize(&m_attribMgr))
        ASSERT(false);
    if (!m_partitioner.initialize(&m_attribMgr, &m_dataMgr))
        ASSERT(false);
    if (!m_evalMgr.initialize(&m_attribMgr, &m_partitioner))
        ASSERT(false);
}

CTDController::~CTDController()
{
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDController::runDiffMulti()
{
	cout << _T("**********************************************************") << endl;
    cout << _T("* Differentially-Private Multidimensional Generalization *") << endl;
    cout << _T("**********************************************************") << endl;

	//printTime();
	time_t time0;
    time(&time0);

	// Read the configuration file for the taxonomy trees of the atrributes
	if (!m_attribMgr.readAttributes())
        return false;

	
    //printTime();
	// Load the data from the file
    if (!m_dataMgr.readRecords())
        return false;
    

	//printTime();
	time_t time1;
    time(&time1);
	cout << _T("Time for reading attributes and records = ") << time1 - time0 << _T(" s") << endl;

	// Anonymize the whole data set
    if (!m_partitioner.transformData())
        return false;
   

	// Add noise to the "training" partitions
	if (!m_partitioner.addNoise())
        return false;
    

	//printTime();
	time_t time2;
    time(&time2);
	cout << _T("Time for transformation and adding noise = ") << time2 - time1 << _T(" s") << endl << endl;


	// Write the .names file for the C4.5 classifier
	// Print the "training" partitions 
#if defined(_TD_NAME_FILE_NORMAL) 
	if (!m_attribMgr.writeNameFile())
		return false;	
	
	if (!m_dataMgr.writeDiffRecords(m_partitioner.getLeafPartitions()))	
		return false; 
#endif

#if defined(_TD_NAME_FILE_MULTIDIM) 
		if (!m_attribMgr.writeNameFileMultiDim())
			return false;	

		if (!m_dataMgr.writeMultiDimRecords(m_partitioner.getLeafPartitions(), m_partitioner.getTestLeafPartitions(), TD_bC45))			
			return false;
#endif

	
    //printTime();
	time_t time3;
    time(&time3);
	cout << _T("Time for writing records = ") << time3 - time2 << _T(" s") << endl << endl;
	

	// Compute Discernibility from noisy leaf partitions
#if defined(_TD_SCORE_FUNTION_DISCERNIBILITY)
	long long catDiscern = 0LL;
	if (!m_evalMgr.countNumDiscern(catDiscern))
		return false;

	cout << _T("Discernibility Penalty = ") << catDiscern << endl << endl;
#endif


	// Compute NCP from noisy leaf partitions
#if defined(_TD_SCORE_FUNCTION_NCP)
	float totalNCP = 0.0f;
	if (!m_evalMgr.countNumTotalNCP(totalNCP))
		return false;

	cout << _T("Total NCP = ") << totalNCP << endl << endl;
#endif
    

	//printTime();
	time_t time4;
    time(&time4);
	cout << _T("Total time = ") << time4 - time0 << _T(" s") << endl;

    return true;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDController::removeUnknowns()
{	
    cout << _T("****************************************") << endl;
    cout << _T("* Removing Records With Unknown Values *") << endl;
    cout << _T("****************************************") << endl;
    
    if (!m_attribMgr.readAttributes())
        return false;

    if (!m_dataMgr.readRecords())
        return false;

    if (!m_dataMgr.writeRecords(true))
        return false;

    return true;
}