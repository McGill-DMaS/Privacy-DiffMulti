// TDConcept.h: interface for the CTDConcept class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDCONCEPT_H)
#define TDCONCEPT_H

class CTDConcept;

typedef CTypedPtrArray<CPtrArray, CTDConcept*> CTDConceptPtrArray;
class CTDConcepts : public CTDConceptPtrArray
{
public:
    CTDConcepts();
    virtual ~CTDConcepts();
	CString toString();

    void cleanup();
};

class CTDAttrib;
class CTDRecords;

class CTDConcept
{
public:
    CTDConcept(CTDAttrib* pAttrib);
    virtual ~CTDConcept();

    virtual bool isContinuous() = 0;
    virtual bool initHierarchy(LPCTSTR conceptStr, int depth, CTDIntArray& maxChildren, int& maxDepth) = 0;
    virtual CString toString() = 0;
    
	bool addChildConcept(CTDConcept* pConceptNode);
    bool addChildConcept(CTDConcept* pConceptNode, int idx);
    int getNumChildConcepts() const;
	int getNumLeafConcepts();
	CTDConcepts * getChildConcepts() {return &m_childConcepts;}
    CTDConcept* getChildConcept(int idx) const;
    CTDConcept* getParentConcept();
    CTDAttrib* getAttrib() { return m_pAttrib; };
	bool computeNCPHelper(float& ncp);

    static bool parseFirstConcept(CString& firstConcept, CString& restStr);

        
// Attributes
    CString        m_conceptValue;          // Actual value in string format.
    int            m_depth;                 // Depth of this concept.
    int            m_childIdx;              // Child index in concept hierarchy.
    int            m_flattenIdx;            // Flattened index in concept hierarchy.
    bool           m_bCutCandidate;         // Can it be a cut candidate?
    POSITION       m_cutPos;                // Position of this concept in the cut.  
	bool		   m_bFileName;				// If true, it will be written to the .names file as an attribute.
	int			   m_nLeafConcepts;			// Number of leaf concepts of this entire hierarchy tree.
	
protected:
// Operations
    virtual bool makeChildConcepts(float splitPoint, CTDConcept*& pLChildCon, CTDConcept*& pRChildCon) = 0;

// Attributes
    CTDAttrib*			m_pAttrib;					// Pointer to this attribute.
    CTDConcept*			m_pParentConcept;
    CTDConcepts			m_childConcepts; 
};


class CTDDiscConcept : public CTDConcept
{
public:
    CTDDiscConcept(CTDAttrib* pAttrib);
    virtual ~CTDDiscConcept();

    virtual bool isContinuous() { return false; };
    virtual bool initHierarchy(LPCTSTR conceptStr, int depth, CTDIntArray& maxBranches, int& maxDepth);
    virtual CString toString();
	
	bool isAncestor(CTDConcept* pTargetConcept);
    static bool parseConceptValue(LPCTSTR str, CString& conceptVal, CString& restStr);

protected:
	virtual bool makeChildConcepts(float splitPoint, CTDConcept*& pLChildCon, CTDConcept*& pRChildCon) {return true;};

// Attributes
    CTDConcept* m_pSplitConcept;            // Pointer to the winner concept.
};


class CTDContConcept : public CTDConcept
{
public:
    CTDContConcept(CTDAttrib* pAttrib);
    virtual ~CTDContConcept();
    virtual bool isContinuous() { return true; };
    virtual bool initHierarchy(LPCTSTR conceptStr, int depth, CTDIntArray& maxBranches, int& maxDepth);
    virtual CString toString();
	virtual bool makeChildConcepts(float splitPoint, CTDConcept*& pLChildCon, CTDConcept*& pRChildCon);
  
    static bool makeRange(float lowerB, float upperB, CString& range);
    static bool parseConceptValue(LPCTSTR  str, 
                                  CString& conceptVal, 
                                  CString& restStr,
                                  float&   lowerBound,
                                  float&   upperBound);
    static bool parseLowerUpperBound(const CString& str, 
                                     float& lowerB, 
                                     float& upperB);

	static char  * FloatToStr (double value, Int16s nDecimals = 0);


// Attributes
    float m_lowerBound;     // Inclusive
    float m_upperBound;     // Exclusive

protected:
// Operations
    bool computeSplitEntropy(float& entropy);

// Attributes
  
};

#endif
