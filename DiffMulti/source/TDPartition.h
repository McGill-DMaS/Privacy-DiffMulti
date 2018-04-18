// TDPartition.h: interface for the CTDPartition class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDPARTITION_H)
#define TDPARTITION_H

#if !defined(TDRECORD_H)
    #include "TDRecord.h"
#endif

#if !defined(TDPARTATTRIB_H)
    #include "TDPartAttrib.h"
#endif

class CTDPartition  
{
public:
    CTDPartition(int partitionIdx, CTDAttribs* pAttribs);
	CTDPartition(int partitionIdx, CTDAttribs* pAttribs, CTDPartition* pParentPartition, int const splitIdx = -1);
    virtual ~CTDPartition();

// operations
    bool initGenRecords(CTDAttribs* pAttribs);
	bool genRecords(CTDPartition*  pParentPartition,
                    CTDAttrib*     pSplitAttrib, 
                    CTDConcept*    pSplitConcept,
					CTDAttribs*    pAttribs,
					int			 childInd);

	int getPartitionIdx() { return m_partitionIdx; };
    CTDPartAttribs* getPartAttribs() { return &m_partAttribs; };
	
    bool addRecord(CTDRecord* pRecord);
    int getNumRecords() { return m_partRecords.GetSize(); };
	int getNumGenRecords() {return m_genRecords.GetSize(); };	
	int getNumClasses() { return m_nClasses; };
    CTDRecord* getRecord(int idx) { return m_partRecords.GetAt(idx); };
    CTDRecords* getAllRecords() { return &m_partRecords; };
	CTDRecords* getGenRecords() { return & m_genRecords; };

   
    bool constructSupportMatrix(double epsilon);
	bool addNoise(double epsilon);
    friend ostream& operator<<(ostream& os, const CTDPartition& partition);

	
	bool computeScore();
	bool pickSpecializeConcept(CTDAttrib*& pSelectedAttrib, CTDConcept*& pSelectedConcept, CTDPartAttrib*& pSelectedPartAttrib, double epsilon);
	bool hasChildConcepts(CTDConcept* pCurrConcept, CTDPartAttrib* pPartAttrib);
	void makeMultiDimAttribs();


	CTDIntArray m_classNoisySums;   // sum of classes	
	POSITION	m_leafPos;		    // Position in the leaf list.
	CTDRecords	m_genRecords;	    // Generalized record.
	int			m_nBudgetCount;		// Accumulated budget usage.
	int			m_nLevelCount;		// Level number in the specialization tree. Root is at Level 0.
	CStringArray m_path;	
	int m_nLocalSpecializations;	// The share of nSpecializations from the parent partition for this partition.


protected:
// attributes
    int m_partitionIdx;
   	CTDPartAttribs m_partAttribs;   // Pointers to attributes of this partition. Does not contain class attr.
    CTDRecords m_partRecords;       // Pointers to records of this partition.    
    int m_nClasses;                 // Number of classes.
	
};


typedef CTypedPtrList<CPtrList, CTDPartition*> CTDPartitionList;
class CTDPartitions : public CTDPartitionList
{
public:
    CTDPartitions();
    virtual ~CTDPartitions();
    void cleanup();

    void deleteEmptyPartitions();

    friend ostream& operator<<(ostream& os, const CTDPartitions& partitions);
};

#endif
