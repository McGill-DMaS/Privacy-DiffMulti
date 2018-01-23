// TDPartAttrib.h: interface for the CTDPartAttrib class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDPARTATTRIB_H)
#define TDPARTATTRIB_H

#include "TDDef.hpp"

#if !defined(TDATTRIBUTE_H)
    #include "TDAttribute.h"
#endif

#if !defined(TDRECORD_H)
    #include "TDRecord.h"
#endif

class CTDPartition;

class CTDPartAttrib  
{
public:
    CTDPartAttrib(CTDAttrib* pActualAttrib);
    virtual ~CTDPartAttrib();

// operations
    bool initSupportMatrix(CTDConcept* pCurrCon, int nClasses);
    
    CTDMDIntArray* getSupportMatrix() { return m_pSupportMatrix; };
    CTDIntArray* getSupportSums() { return &m_supportSums; };
    CTDIntArray* getClassSums() { return &m_classSums; };
    CTDAttrib* getActualAttrib() { return m_pActualAttrib; };


	bool computeScore(int nClasses, CTDConcept* pCurrCon);
	bool computeMaxHelper(const CTDIntArray& supSums, 
                                 const CTDIntArray& classSums,
								 CTDMDIntArray& supMatrix,
                                 float& infoGainDiff);
	bool computeInfoGainHelper(float entropy, 
                                const CTDIntArray& supSums, 
                                const CTDIntArray& classSums, 
                                CTDMDIntArray& supMatrix,
                                float& infoGainDiff);
	bool computeDiscernHelper(const CTDIntArray& supSums,
								long long& discern);
	bool computeNCPHelper(const CTDIntArray& supSums, 
								float& ncp, CTDConcept* pCurrCon);
	bool computeNCPSplitHelper(const CTDIntArray& supSums, 
								float& ncp,
								CTDNumericValue* pCurrValue,
								CTDNumericValue* pNextValue,
								CTDContConcept*  pCurrConcept);
	bool computeNCPHelperHelper(float& ncp, CTDConcept* pCurrCon);

    static float computeEntropy(CTDIntArray* pClassSums);
	inline float log2f(float x) { return log10f(x) / log10f(2); };
	bool divideConcept(double epsilon, int nClasses, CTDConcept* pCurrConcept, CTDPartition* pCurrPartition);
	bool findOptimalSplitPoint(CTDRecords& recs, int nClasses, double epsilon, CTDConcept* pCurrConcept = NULL);
	bool initSplitMatrix(int nConcepts, int nClasses);
	float getSplitPoint() { return m_splitPoint; };

// attributes
    bool           m_bCandidate;
	float		   m_splitPoint;		// Split point of the concept of this partAttrib.
	float          m_infoGain;          // Information gain (Score)
	float		   m_max;				// Max (Score)
	long long	   m_discern;			// Discernibility (Score)
	float		   m_ncp;				// Normalized Certainty Penalty (Score)
	float		   m_totalNCP;			// Used in CTDEvalMgr::countNumTotalNCP.
	CTDAttrib*     m_pActualAttrib;    
	CTDConcept*	   m_pLeftChildCon;		// Used when dividing a continuous concept.
	CTDConcept*    m_pRightChildCon;	// Pointers to newly created child concepts.
    

protected:
// attributes
	
	// Matrices used when computing the score
    CTDMDIntArray* m_pSupportMatrix;    // raw count of supports
    CTDIntArray    m_supportSums;       // sum of supports of each concept
    CTDIntArray    m_classSums;         // sum of classes
	
	// Matrices used when determining a split point of a cont concept
	CTDMDIntArray* m_pSplitSupMatrix;   // raw count of supports
    CTDIntArray    m_splitSupSums;      // sum of supports of each child concept
    CTDIntArray    m_splitClassSums;    // sum of classes
};


typedef CTypedPtrList<CPtrList, CTDPartAttrib*> CTDPartAttribList;
class CTDPartAttribs : public CTDPartAttribList
{
public:
    CTDPartAttribs();
    virtual ~CTDPartAttribs();
};


//------------------------------------------------------------------------------------

class CTDRange;

typedef CTypedPtrArray<CPtrArray, CTDRange*> CTDRangePtrArray;
class CTDRanges : public CTDRangePtrArray
{
public:
    CTDRanges();
    virtual ~CTDRanges();

    void cleanup();
};


class CTDRange
{
public:
    CTDRange(float upperValue, float lowerValue);
    virtual ~CTDRange();

	// attributes
    float          m_upperValue;      
    float          m_lowerValue;      
};


#endif
