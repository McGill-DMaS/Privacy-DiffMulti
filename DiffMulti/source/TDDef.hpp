// TDDef.hpp: define the constants/types shared among all classes
//
//////////////////////////////////////////////////////////////////////

#if !defined(TDDEF_HPP)
#define TDDEF_HPP

// Pre-processor
#ifdef _DEBUG
    #define _DEBUG_PRT_INFO
#endif
//#define _DEBUG_PRT_INFO				
										
										

#define DEBUGPrint _tprintf				// Print in console
//#define DEBUGPrint TRACE				// Print in debug window
//#define _TD_MANUAL_CONTHRCHY
//#define _TD_TREAT_CONT_AS_CONT		// Treat continuous attributes as continuous attributes in C4.5


// Score functions
//#define _TD_SCORE_FUNCTION_INFOGAIN	// Change the name file below to _TD_NAME_FILE_MULTIDIM
#define _TD_SCORE_FUNTION_MAX			// Change the name file below to _TD_NAME_FILE_MULTIDIM
//#define _TD_SCORE_FUNTION_DISCERNIBILITY
//#define _TD_SCORE_FUNCTION_NCP


// Name file
//#define _TD_NAME_FILE_NORMAL			// Original attributes with generalized domain values.
#define _TD_NAME_FILE_MULTIDIM			// Any generalized concept is considered an attribute with domain values = {0, 1}, except numerical attributes.
										// Used for classifying a multidimensionally-generalized data set.
										// Activate for _TD_SCORE_FUNCTION_INFOGAIN or _TD_SCORE_FUNTION_MAX.

// Classifier type: C.45 or SVM-light
#define TD_bC45								0	// Insert a boolean value. 0 for SVM data format.
												// Used only when _TD_NAME_FILE_MULTIDIM.


#define P_ASSERT(p) if (!p) { ASSERT(false); return false; }

// Common types
typedef CArray<int, int>			CTDIntArray;
typedef CArray<float, float>		CTDFloatArray;
typedef CArray<bool, bool>			CTDBoolArray;
typedef CArray<POSITION, POSITION>	CTDPosArray;	
typedef CBFMultiDimArray<int> CTDMDIntArray;


// Constants
#define TD_RAWDATAFILE_EXT                  _T("rawdata")
#define TD_ATTRBFILE_EXT                    _T("hchy")
#define TD_NAMEFILE_EXT                     _T("names")
#define TD_TRANSFORM_DATAFILE_EXT           _T("data")
#define TD_TRANSFORM_TESTFILE_EXT           _T("test")

#define TD_VID_ATTRIB_NAME                  _T("vid")
#define TD_CLASSES_ATTRIB_NAME              _T("classes")
#define TD_DISCRETE_ATTRIB                  _T("discrete")
#define TD_CONTINUOUS_ATTRIB                _T("continuous")
#define TD_MASKTYPE_GEN                     _T("generalization")
#define TD_MASKTYPE_SUP                     _T("suppression")

#define TD_TRANSACTION_ITEM_PRESENT         _T("1")

#define TD_CONHCHY_OPENTAG                  TCHAR('{')
#define TD_CONHCHY_CLOSETAG                 TCHAR('}')
#define TD_CONHCHY_DASHSYM                  TCHAR('-')
#define TD_CONHCHY_COMMENT                  TCHAR('|')
#define TD_RAWDATA_DELIMETER                TCHAR(',')
#define TD_SETVALUE_DELIMETER               TCHAR('>')
#define TD_RAWDATA_TERMINATOR               TCHAR('.')
#define TD_UNKNOWN_VALUE                    TCHAR('?')

#define TD_NAMEFILE_ATTNAMESEP              TCHAR(':')
#define TD_NAMEFILE_SEPARATOR               TCHAR(',')
#define TD_NAMEFILE_TERMINATOR              TCHAR('.')
#define TD_NAMEFILE_CONTINUOUS              _T("continuous")
#define TD_NAMEFILE_FAKE_CONT_CONCEPT       _T("fake")

#define TD_CONTVALUE_NUMDEC                 2

// Normalized Discernibility range
#define TD_NORM_LOWER_BOUND					0
#define TD_NORM_UPPER_BOUND					10000

// Original Discernibility range
#define	TD_DISCERN_LOWER_BOUND				0.0
#define	TD_DISCERN_UPPER_BOUND				909746244.0	// square(30162). 
														// In the worst case, a parent partition would contain the entire training records (30162)
														// and its child partitions contain: 30162 recs, 0 recs.
														// Discern for the parent is square(30162) + square(0) = 909746244.
														// No Discern will be higher than that number.

// NCP
#define	TD_MAX_NORMNCP_VALUE				100	// 0 <= ncp <= 1. 
												// Lower ncp values imply less generalizations.
												// ExpMech() favors higher values. Therefore, we want to favor lower ncp values.
												// To do so, NormNCP = 1 / ncp.
												// Must avoid the case of 1 / 0.
												// For the Adult data set, the next lowest ncp value after 0 is ncp = 1 / 41 (native-country).
												// ncp = 1 / 41 ---> NormNCP = 41.
												// Therefore, 100 is chosen as a reasonably larger value. ncp = 0 --> NormNCP = 100.


#define	TD_CONST							10000		// To mitigate the effect of the added Laplace noise, especially for small counts.
														// Data independent


// For computing longest path
#define TD_CONT_ATTR_LEVELS					7	// Estimated number of levels in a continuous hierarchy.

#endif