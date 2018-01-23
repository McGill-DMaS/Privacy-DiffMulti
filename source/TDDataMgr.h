// TDDataMgr.h: interface for the CTDDataMgr class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDDATAMGR_H)
#define TDDATAMGR_H

#if !defined(TDATTRIBMGR_H)
    #include "TDAttribMgr.h"
#endif


#if !defined(TDRECORD_H)
    #include "TDRecord.h"
#endif


#if !defined(TDPARTITION_H)
    #include "TDPartition.h"
#endif

//Class CTDPartitions;

class CTDDataMgr  
{
public:
    CTDDataMgr(LPCTSTR rawDataFile, LPCTSTR transformedDataFile, LPCTSTR transformedTestFile, int nInputRecs, int nTraining);
    virtual ~CTDDataMgr();

// Operations
    bool initialize(CTDAttribMgr* pAttribMgr);
    bool readRecords();
    bool writeRecords(bool bRawValue);
    bool writeDiffRecords(CTDPartitions* pLeafPartitions);
	bool writeMultiDimRecords(CTDPartitions* pLeafPartitions, CTDPartitions* pTestLeafPartitions, bool isC45, bool isCluster);
	bool writeVectorRecordsForVCluster(CStdioFile& dataFile);
	int  countNumVectorAttribs(bool bIncludeClassAttr);
    CTDRecords* getRecords() { return &m_records; };
	CTDRecords* getTestRecords() { return &m_testRecords; };
	void convertRecord(CTDPartition* pLeafPartition, bool isC45, bool isTestPartition, CString& str);
	void printPath(CTDPartition* pLeafPartition);

	//double StrToFloat (const char * string);
    
protected:
    bool addRecord(CTDRecord* pRecord);

// Attributes
    CString         m_rawDataFile;
    CString         m_transformedDataFile;
    CString         m_transformedTestFile;
    CTDRecords      m_records;		// Only training records.
	CTDRecords      m_testRecords;	// Only testing records.
    CTDAttribMgr*   m_pAttribMgr;
	int             m_nInputRecs;	// Number of all records in input data set.
    int             m_nTraining;
};

#endif
