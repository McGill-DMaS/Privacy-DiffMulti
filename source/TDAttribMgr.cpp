// TDAttribMgr.cpp: implementation of the CTDAttribMgr class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDATTRIBMGR_H)
    #include "TDAttribMgr.h"
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTDAttribMgr::CTDAttribMgr(LPCTSTR attributesFile, LPCTSTR nameFile) 
    : m_attributesFile(attributesFile), m_nameFile(nameFile), m_numConAttrib(0)
{
}

CTDAttribMgr::~CTDAttribMgr() 
{
    m_attributes.cleanup();
}

//---------------------------------------------------------------------------
// Read information from the attribute hierachy file and build the concept hierarchy

//---------------------------------------------------------------------------
bool CTDAttribMgr::readAttributes()
{
    cout << _T("Reading attributes...") << endl;
	m_attributes.cleanup();
    try {
        CStdioFile attribFile;
        if (!attribFile.Open(m_attributesFile, CFile::modeRead)) {
            cerr << _T("CTDAttribMgr: Failed to open file ") << m_attributesFile << endl;
            return false;
        }

        // Parse each line       
        CString lineStr, attribName, attribType, attribValuesStr;
        int commentCharPos = -1, semiColonPos = -1;
        bool bMaskTypeSuppress = false;
        CTDAttrib* pClassAttribute = NULL;
        while (attribFile.ReadString(lineStr)) {
            CBFStrHelper::trim(lineStr);
            if (lineStr.IsEmpty())
                continue;

            // Remove comments
            commentCharPos = lineStr.Find(TD_CONHCHY_COMMENT);
            if (commentCharPos != -1) {
                lineStr = lineStr.Left(commentCharPos);
                CBFStrHelper::trim(lineStr);
                if (lineStr.IsEmpty())
                    continue;
            }

            // Find semicolon
            semiColonPos = lineStr.Find(TCHAR(':'));
            if (semiColonPos == -1) {
                cerr << _T("CTDAttribMgr: Unknown line: ") << lineStr << endl;
                ASSERT(false);
                return false;
            }

            // Extract attribute name
            attribName = lineStr.Left(semiColonPos);
            CBFStrHelper::trim(attribName);
            if (attribName.IsEmpty()) {
                cerr << _T("CTDAttribMgr: Invalid attribute: ") << lineStr << endl;
                ASSERT(false);
                return false;
            }
            
            // Find semicolon
            lineStr = lineStr.Mid(semiColonPos + 1);
            CBFStrHelper::trim(lineStr);
            semiColonPos = lineStr.Find(TCHAR(':'));
            if (semiColonPos == -1) {
                attribType = lineStr;
                bMaskTypeSuppress = false;
            }
            else {
                // Extract attribute type
                attribType = lineStr.Left(semiColonPos);
                CBFStrHelper::trim(attribType);
                if (attribType.IsEmpty()) {
                    cerr << _T("CTDAttribMgr: Invalid attribute type: ") << lineStr << endl;
                    ASSERT(false);
                    return false;
                }

                // Extract mask type
                lineStr = lineStr.Mid(semiColonPos + 1);
                CBFStrHelper::trim(lineStr);
                bMaskTypeSuppress = lineStr.CompareNoCase(TD_MASKTYPE_SUP) == 0;
            }
			
			// Count the number of continuous attributes
			if (attribType.CompareNoCase(TD_CONTINUOUS_ATTRIB) == 0)
				++m_numConAttrib;
            
			if (bMaskTypeSuppress && attribType.CompareNoCase(TD_CONTINUOUS_ATTRIB) == 0) {
                cerr << _T("CTDAttribMgr: Continuous attribute cannot be suppression.") << endl;
                ASSERT(false);
                return false;
            }

            if (bMaskTypeSuppress && attribName.CompareNoCase(TD_CLASSES_ATTRIB_NAME) == 0) {
                cerr << _T("CTDAttribMgr: Classes cannot be suppression.") << endl;
                ASSERT(false);
                return false;
            }

            if (attribName.CompareNoCase(TD_CLASSES_ATTRIB_NAME) == 0 && 
                attribType.CompareNoCase(TD_CONTINUOUS_ATTRIB) == 0) {
                cerr << _T("CTDAttribMgr: Classes cannot be continuous.") << endl;
                ASSERT(false);
                return false;
            }

            // Read the next line which contains the hierarchy
            if (!attribFile.ReadString(attribValuesStr)) {
                cerr << _T("CTDAttribMgr: Invalid attribute: ") << attribName << endl;
                ASSERT(false);
                return false;
            }

            CBFStrHelper::trim(attribValuesStr);
            if (attribValuesStr.IsEmpty()) {
                cerr << _T("CTDAttribMgr: Invalid attribute: ") << attribName << endl;
                ASSERT(false);
                return false;
            }

            // Remove comments
            commentCharPos = attribValuesStr.Find(TD_CONHCHY_COMMENT);
            if (commentCharPos != -1) {
                attribValuesStr = attribValuesStr.Left(commentCharPos);
                CBFStrHelper::trim(attribValuesStr);
                if (attribValuesStr.IsEmpty()) {
                    cerr << _T("CTDAttribMgr: Invalid attribute: ") << attribName << endl;
                    ASSERT(false);
                    return false;
                }
            }

			if (attribName.CompareNoCase(TD_VID_ATTRIB_NAME) != 0) {
                // Create an attribute
				ASSERT(attribType == TD_CONTINUOUS_ATTRIB || attribType == TD_DISCRETE_ATTRIB);
                CTDAttrib* pNewAttribute = NULL;
                if (attribType.CompareNoCase(TD_CONTINUOUS_ATTRIB) == 0)
                    pNewAttribute = new CTDContAttrib(attribName);
                else if (attribType.CompareNoCase(TD_DISCRETE_ATTRIB) == 0)
                    pNewAttribute = new CTDDiscAttrib(attribName, bMaskTypeSuppress);
				

                if (!pNewAttribute) {
                    ASSERT(false);
                    return false;
                }
                if (!pNewAttribute->initHierarchy(attribValuesStr)) {
                    cerr << _T("CTDAttribMgr: Failed to build hierarchy for ") << attribName << endl;
                    return false;
                }

                if (attribName.CompareNoCase(TD_CLASSES_ATTRIB_NAME) != 0) {
                    // Add the attribute to the attribute array
                    pNewAttribute->m_attribIdx = m_attributes.Add(pNewAttribute);
					pNewAttribute->m_bVirtualAttrib = true;  

					// Find the num of leaf concepts
					pNewAttribute->getConceptRoot()->m_nLeafConcepts = pNewAttribute->getConceptRoot()->getNumLeafConcepts();
                }
                else {
                    ASSERT(!pClassAttribute);
                    pClassAttribute = pNewAttribute;
                }
            }
        }

        // Add class attribute to the end of the attribute array
        if (!pClassAttribute) {
            cerr << _T("CTDAttribMgr: Missing classes.") << endl;
            ASSERT(false);
            return false;
        }
        pClassAttribute->m_attribIdx = m_attributes.Add(pClassAttribute);
        attribFile.Close();
    }
    catch (CFileException&) {
        cerr << _T("Failed to read attributes file: ") << m_attributesFile << endl;
        ASSERT(false);
        return false;
    }
    cout << m_attributes;
    cout << _T("Reading attributes succeeded.") << endl;

    return true;
}

//---------------------------------------------------------------------------
// Create a name file for C4.5.
//---------------------------------------------------------------------------
bool CTDAttribMgr::writeNameFile()
{
    cout << _T("Writing name file...") << endl;
    try {
        CStdioFile nameFile;
        if (!nameFile.Open(m_nameFile, CFile::modeCreate | CFile::modeWrite)) {
            cerr << _T("CTDAttribMgr: Failed to open file ") << m_nameFile << endl;
            return false;
        }

        // Write class.
		int c = 0;
        int nAttributes = getNumAttributes();
        CTDAttrib* pAttrib = m_attributes.GetAt(nAttributes - 1);
        CTDConcepts* pFlattenConcepts = pAttrib->getFlattenConcepts();
        for (c = 1; c < pFlattenConcepts->GetSize(); ++c) {
            nameFile.WriteString(pFlattenConcepts->GetAt(c)->m_conceptValue);
            if (c < pFlattenConcepts->GetSize() - 1)
                nameFile.WriteString(CString(TD_NAMEFILE_SEPARATOR) + _T(" "));
            else {
                nameFile.WriteString(CString(TD_NAMEFILE_TERMINATOR));
                nameFile.WriteString(_T("\n\n"));
            }
        }

        // Write attributes
        for (int a = 0; a < nAttributes - 1; ++a) {
            pAttrib = m_attributes.GetAt(a);
            nameFile.WriteString(pAttrib->m_attribName + TD_NAMEFILE_ATTNAMESEP + _T(" "));

            pFlattenConcepts = pAttrib->getFlattenConcepts();
#ifdef _TD_TREAT_CONT_AS_CONT
            if (pAttrib->isContinuous()) {
#else
            if (pAttrib->isContinuous() && !pAttrib->m_bVirtualAttrib) {
#endif         
                nameFile.WriteString(TD_NAMEFILE_CONTINUOUS);
                nameFile.WriteString(_T("\n"));
            }
            else {
                int nConcepts = 0;
                if (pAttrib->isMaskTypeSup())
                    nConcepts = pAttrib->m_nOFlatConcepts;
                else
                    nConcepts = pFlattenConcepts->GetSize();                  

                for (c = 0; c < nConcepts; ++c) {
                    nameFile.WriteString(pFlattenConcepts->GetAt(c)->m_conceptValue);
                    if (c < nConcepts - 1)
                        nameFile.WriteString(CString(TD_NAMEFILE_SEPARATOR) + _T(" "));
                    else {
                        if (pFlattenConcepts->GetSize() == 1) {
                            // add a fake concept if there is only one concept.
                            nameFile.WriteString(CString(TD_NAMEFILE_SEPARATOR) + _T(" "));
                            nameFile.WriteString(CString(TD_NAMEFILE_FAKE_CONT_CONCEPT));
                        }
                        nameFile.WriteString(CString(TD_NAMEFILE_TERMINATOR));
                        nameFile.WriteString(_T("\n"));
                    }
                }
            }
        }
        nameFile.Close();
    }
    catch (CFileException&) {
        cerr << _T("Failed to write name file: ") << m_nameFile << endl;
        ASSERT(false);
        return false;
    }
    cout << _T("Writing name file succeeded.") << endl << endl;
    return true;
}

//---------------------------------------------------------------------------
// Create a name file for C4.5 taking into account the multidimensional anonymization.
// This .names file has every categorical concept as a distrinct attribute,
// with domain values {0, 1}.
// For continuous attributes, Age: continuous.
//---------------------------------------------------------------------------
bool CTDAttribMgr::writeNameFileMultiDim()
{
    cout << _T("Writing multidimensional name file...") << endl;
    try {
        CStdioFile nameFile;
        if (!nameFile.Open(m_nameFile, CFile::modeCreate | CFile::modeWrite)) {
            cerr << _T("CTDAttribMgr: Failed to open file ") << m_nameFile << endl;
            return false;
        }

        // Write class
		int c = 0;
        int nAttributes = getNumAttributes();
        CTDAttrib* pAttrib = m_attributes.GetAt(nAttributes - 1);
        CTDConcepts* pFlattenConcepts = pAttrib->getFlattenConcepts();
        for (c = 1; c < pFlattenConcepts->GetSize(); ++c) {
            nameFile.WriteString(pFlattenConcepts->GetAt(c)->m_conceptValue);
            if (c < pFlattenConcepts->GetSize() - 1)
                nameFile.WriteString(CString(TD_NAMEFILE_SEPARATOR) + _T(" "));
            else {
                nameFile.WriteString(CString(TD_NAMEFILE_TERMINATOR));
                nameFile.WriteString(_T("\n\n"));
            }
        }

        // Write attributes
        for (int a = 0; a < nAttributes - 1; ++a) {
            pAttrib = m_attributes.GetAt(a);
			pFlattenConcepts = pAttrib->getFlattenConcepts();
			int nConcepts = 0;
			if (pAttrib->isMaskTypeSup())
                nConcepts = pAttrib->m_nOFlatConcepts;
            else
                nConcepts = pFlattenConcepts->GetSize();    

			if (pAttrib->isContinuous()) {
				nameFile.WriteString(pAttrib->m_attribName + TD_NAMEFILE_ATTNAMESEP + _T(" "));
				nameFile.WriteString(CString(TD_NAMEFILE_CONTINUOUS));
				nameFile.WriteString(CString(TD_NAMEFILE_TERMINATOR));
                nameFile.WriteString(_T("\n"));
				continue;
			}
			else {
				for (c = 0; c < nConcepts; ++c) {
					if (pFlattenConcepts->GetAt(c)->m_bFileName) {
						nameFile.WriteString(pFlattenConcepts->GetAt(c)->m_conceptValue + TD_NAMEFILE_ATTNAMESEP + _T(" 0, 1"));
						nameFile.WriteString(CString(TD_NAMEFILE_TERMINATOR));
						nameFile.WriteString(_T("\n"));

						// Concept is a multidimensional attribute
						pAttrib->m_multiDimConcepts.Add(pFlattenConcepts->GetAt(c));
					}
				} 
			}
        }
        nameFile.Close();
    }
    catch (CFileException&) {
        cerr << _T("Failed to write multidimensional name file: ") << m_nameFile << endl;
        ASSERT(false);
        return false;
    }
    cout << _T("Writing multidimensional name file succeeded.") << endl << endl;
    return true;
}

