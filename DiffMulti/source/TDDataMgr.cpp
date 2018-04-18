// TDDataMgr.cpp: implementation of the CTDDataMgr class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"

#if !defined(TDDATAMGR_H)
    #include "TDDataMgr.h"
#endif

#if !defined(TDPARTITION_H)
    #include "TDPartition.h"
#endif


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTDDataMgr::CTDDataMgr(LPCTSTR rawDataFile, LPCTSTR transformedDataFile, LPCTSTR transformedTestFile, int nInputRecs, int nTraining) 
    : m_rawDataFile(rawDataFile), 
      m_transformedDataFile(transformedDataFile), 
      m_transformedTestFile(transformedTestFile), 
      m_nInputRecs(nInputRecs),
      m_nTraining(nTraining)
{
}

CTDDataMgr::~CTDDataMgr() 
{
    m_records.cleanup();
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDDataMgr::initialize(CTDAttribMgr* pAttribMgr)
{
    if (!pAttribMgr) {
        ASSERT(false);
        return false;
    }
    m_pAttribMgr = pAttribMgr;
    return true;
}

//---------------------------------------------------------------------------
// Read records from raw data file
//---------------------------------------------------------------------------
bool CTDDataMgr::readRecords()
{
    cout << _T("Reading records...") << endl;
    m_records.cleanup();
    CTDAttribs* pAttribs = m_pAttribMgr->getAttributes();
    try {
        CStdioFile rawFile;
        if (!rawFile.Open(m_rawDataFile, CFile::modeRead)) {
            cerr << _T("CTDDataMgr: Failed to open file ") << m_rawDataFile << endl;
            return false;
        }

        // Parse each line
        int commentCharPos = -1;
        CString lineStr;
        while (rawFile.ReadString(lineStr)) {
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

            // Remove period at the end of the line
            if (lineStr[lineStr.GetLength() - 1] == TD_RAWDATA_TERMINATOR) {
                lineStr = lineStr.Left(lineStr.GetLength() - 1);
                CBFStrHelper::trim(lineStr);
                if (lineStr.IsEmpty())
                    continue;
            }
   
            int attribID = 0;
            CString valueStr;
            CTDAttrib* pAttrib = NULL;
            CTDValue* pNewValue = NULL;
            CTDRecord* pNewRecord = new CTDRecord();                        
            CBFStrParser strParser(lineStr, TD_RAWDATA_DELIMETER);
            while (strParser.getNext(valueStr)) {
                // Check unknown value
				CBFStrHelper::trim(valueStr);
				if (valueStr.IsEmpty()) {
                    cerr << _T("CTDDataMgr: Empty value string in record: ") << lineStr << endl;
                    ASSERT(false);
                    return false;
                }
                if (valueStr == TD_UNKNOWN_VALUE) {
                    // Discard this record
                    delete pNewRecord;
                    pNewRecord = NULL;
                    break;
                }

                // Allocate a new value
                pNewValue = NULL;
                pAttrib = pAttribs->GetAt(attribID);

                if (pAttrib->isContinuous())
                    pNewValue = new CTDNumericValue((float) StrToFloat(valueStr));
               	else
                    pNewValue = new CTDStringValue();

                if (!pNewValue) {
                    ASSERT(false);
                    return false;
                }

				
                // Match the value to the lowest concept
                // Then build the bit value in case of categorical attribute
                if (!pNewValue->buildBitValue(valueStr, pAttrib)) {	
                    cerr << _T("CTDDataMgr: Failed to build bit value: ") << valueStr
                         << _T(" in attribute ") << pAttrib->m_attribName << endl;                    
                    ASSERT(false);
                    return false;
                }

                if (attribID == pAttribs->GetSize() - 1) {
                    // Class attribute
                    if (!pNewValue->initConceptToLevel1(pAttrib))
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

                ++attribID;
            }

            if (pNewRecord) {
				
				if (m_records.GetSize() >= m_nTraining)
					pNewRecord->setRecordID(m_testRecords.Add(pNewRecord));
				else
					pNewRecord->setRecordID(m_records.Add(pNewRecord));
            }

            // Read in the specified number of records
			if (m_nInputRecs >= 0 && (m_records.GetSize()+ m_testRecords.GetSize()) >= m_nInputRecs)
                break;
        }
        rawFile.Close();

        if (m_records.GetSize() == 0) {
            cerr << _T("CTDDataMgr: No records.") << endl;
            return false;
        }

		 if (m_testRecords.GetSize() == 0) {
            cerr << _T("CTDDataMgr: No test records.") << endl;
            return false;
        }
    }
    catch (CFileException&) {
        cerr << _T("Failed to read raw data file: ") << m_rawDataFile << endl;
        ASSERT(false);
        return false;
    }

#ifdef _DEBUG_PRT_INFO
    cout << _T("Number of Records = ") << (m_records.GetSize()+ m_testRecords.GetSize())<< endl;
    cout << endl;
#endif

    cout << _T("Reading records succeeded.") << endl;
    return true;
}


//---------------------------------------------------------------------------
// Write records to transformed data file. 
//This method is used to write the raw data without missing values.
//---------------------------------------------------------------------------
bool CTDDataMgr::writeRecords(bool bRawValue)
{
    cout << _T("Writing records...") << endl;
    try {
        // Write data file.
        CStdioFile transDataFile;
        if (!transDataFile.Open(m_transformedDataFile, CFile::modeCreate | CFile::modeWrite)) {
            cerr << _T("CTDDataMgr: Failed to open file ") << m_transformedDataFile << endl;
            return false;
        }

        int i = 0;
        int nRecords = m_records.GetSize();
        if (m_nTraining > nRecords) {
            cerr << _T("CTDDataMgr: Number of training records must be <= number of records in rawdata.") << m_transformedDataFile << endl;
            ASSERT(false);
            return false;
        }
        for (i = 0; i < m_nTraining; ++i) {
            if (bRawValue)
                transDataFile.WriteString(m_records.GetAt(i)->toString(bRawValue) + _T("\n"));
            else
                transDataFile.WriteString(m_records.GetAt(i)->toString(bRawValue) + _T("\n"));
        }
        transDataFile.Close();

        // Write test file
        CStdioFile transTestFile;
        if (!transTestFile.Open(m_transformedTestFile, CFile::modeCreate | CFile::modeWrite)) {
            cerr << _T("CTDDataMgr: Failed to open file ") << m_transformedTestFile << endl;
            return false;
        }

        for (i = m_nTraining; i < nRecords; ++i) {
            if (bRawValue)
                transTestFile.WriteString(m_records.GetAt(i)->toString(bRawValue) + _T("\n"));
            else
                transTestFile.WriteString(m_records.GetAt(i)->toString(bRawValue) + _T("\n"));
        }
        transTestFile.Close();
    }
    catch (CFileException&) {
        cerr << _T("Failed to write transformed data file: ") << m_transformedDataFile << endl;
        ASSERT(false);
        return false;
    }
    cout << m_records;
    cout << _T("Writing records succeeded.") << endl << endl;
    return true;
}


//---------------------------------------------------------------------------
// Write records to transformed data file in the differential privacy format
//---------------------------------------------------------------------------
bool CTDDataMgr::writeDiffRecords(CTDPartitions* pLeafPartitions)
{
    cout << _T("Writing records...") << endl;
	int longestPath = 0;

    try {
		// Write data file
        CStdioFile transDataFile;
        if (!transDataFile.Open(m_transformedDataFile, CFile::modeCreate | CFile::modeWrite)) {
            cerr << _T("CTDDataMgr: Failed to open file ") << m_transformedDataFile << endl;
            return false;
        }
		CTDPartition* pLeafPartition = NULL;
		for (POSITION childPos = pLeafPartitions->GetHeadPosition(); childPos != NULL;) {
			pLeafPartition = pLeafPartitions->GetNext(childPos);

			if ( pLeafPartition->getNumGenRecords() != pLeafPartition->getNumClasses()) {
				cerr << _T("Number of generalized records is not same as the number of classes") << endl;
				ASSERT(false);
				return false;
			}
			CString str;
			for (int j = 0; j < pLeafPartition->getNumClasses(); ++j){
				str = pLeafPartition->getGenRecords()->GetAt(j)->toString(false) ;	// "false" returns m_pCurrConcept ("true" returns m_pRawConcept)
				// Print the generalized records according to the noisyCount
				for (int i = 0; i < pLeafPartition->m_classNoisySums[j]; ++i) 
					transDataFile.WriteString(str + _T("\n"));
			}

#ifdef _DEBUG_PRT_INFO
			printPath(pLeafPartition);
#endif

			if (pLeafPartition->m_path.GetSize() > longestPath)
				longestPath = pLeafPartition->m_path.GetSize() - 1; // Root-to-child has path length = 1.
			
		}
		transDataFile.Close();

        // Write test file
        CStdioFile transTestFile;
        if (!transTestFile.Open(m_transformedTestFile, CFile::modeCreate | CFile::modeWrite)) {
            cerr << _T("CTDDataMgr: Failed to open file ") << m_transformedTestFile << endl;
            return false;
        }
		int nRecords = m_testRecords.GetSize();
        for (int i = 0; i < nRecords; ++i) {
			transTestFile.WriteString(m_testRecords.GetAt(i)->toString(false) + _T("\n"));
        }
        transTestFile.Close();
    }
    catch (CFileException&) {
        cerr << _T("Failed to write transformed data file: ") << m_transformedDataFile << endl;
        ASSERT(false);
        return false;
    }
   
	cout << _T("Longest root-to-leaf path is: ") << longestPath << endl;
    cout << _T("Writing records succeeded.") << endl << endl;
    return true;
}

//---------------------------------------------------------------------------
// If isC45 == 1, print to C.45 classifier. Else, print to SVM classifier.
// C4.5 data format: <value>,...,<value>,<class_label>
// SVM data format: <class_label> <concept#>:<value> <concept#>:<value>
// Where:
// value = 0 or 1 for categorical attributes. For numerical attributes:
// 1) for C4.5: midpoint of the interval to which the raw numerical value has been generalized.
// 2) for SVM: normalized raw value within the full numerical domain ([0-1]). However, raw values
//		are being generalized to intervals in our approach. Hence, the generalization interval
//		is normalized with respect to the full domain range.
// concept# is an integer >= 1 representing the index of a concept value
//		within the set of all concept values in all leaf partitions. 
//		concept# should always be increasing.
// class_label: the actual class label for C4.5, or +1 or -1 for SVM.
//---------------------------------------------------------------------------
bool CTDDataMgr::writeMultiDimRecords(CTDPartitions* pLeafPartitions, CTDPartitions* pTestLeafPartitions, bool isC45)
{
	cout << _T("Writing multidimensional records with preprocessing for classifier...") << endl;
	int longestPath = 0;

    try {
	
		// Write data file.
        CStdioFile transDataFile;
		if (!transDataFile.Open(m_transformedDataFile, CFile::modeCreate | CFile::modeWrite)) {
            cerr << _T("CTDDataMgr: Failed to open file ") << m_transformedDataFile << endl;
            return false;
        }

		for (POSITION leafPos = pLeafPartitions->GetHeadPosition(); leafPos != NULL;) {
			CTDPartition* pLeafPartition = NULL;
			pLeafPartition = pLeafPartitions->GetNext(leafPos);
			if ( pLeafPartition->getNumGenRecords() != pLeafPartition->getNumClasses()) {
				cerr << _T("Number of generalized records is not same as the number of classes") << endl;
				ASSERT(false);
				return false;
			}

			CString str;
			str.Empty();
			convertRecord(pLeafPartition, isC45, 0, str);

			// Print the multidimensionally generalized records according to their pertinent noisyCounts in this leafPartition
			CString classValue;
			int nClasses = pLeafPartition->getNumClasses();
			int classIdx = pLeafPartition->getPartAttribs()->GetSize();
			for (int j = 0; j < nClasses; ++j){
				// Obtain the class value
				classValue = pLeafPartition->getGenRecords()->GetAt(j)->getValue(classIdx)->toString(true);
				if (!isC45) {
					if (classValue == ">50K")
						classValue = "+1 ";
					else
						classValue = "-1 ";
				}
				for (int i = 0; i < pLeafPartition->m_classNoisySums[j]; ++i) {
					if (isC45)
						transDataFile.WriteString(str + classValue + TD_RAWDATA_TERMINATOR + _T("\n"));
					else 
						transDataFile.WriteString(classValue + str + _T("\n"));
				}
			}
			
#ifdef _DEBUG_PRT_INFO
			printPath(pLeafPartition);
#endif

			if (pLeafPartition->m_path.GetSize() > longestPath)
				longestPath = pLeafPartition->m_path.GetSize() - 1;	// Root-to-child has path length = 1.
		}
		transDataFile.Close();
		// Finish data file

        // Write test file
        CStdioFile transTestFile;
        if (!transTestFile.Open(m_transformedTestFile, CFile::modeCreate | CFile::modeWrite)) {
            cerr << _T("CTDDataMgr: Failed to open file ") << m_transformedTestFile << endl;
            return false;
        }

		for (POSITION leafPos = pTestLeafPartitions->GetHeadPosition(); leafPos != NULL;) {
			CTDPartition* pTestLeafPartition = NULL;
			pTestLeafPartition = pTestLeafPartitions->GetNext(leafPos);

			CString str;
			str.Empty();
			convertRecord(pTestLeafPartition, isC45, 1, str);

			// Print the multidimensionally generalized test records according to their raw counts in this testLeafPartition
			CString classValue;
			int nRecrods = pTestLeafPartition->getNumRecords();
			int classIdx = pTestLeafPartition->getPartAttribs()->GetSize();
			for (int j = 0; j < nRecrods; ++j) {
				// obtain the class value
				classValue = pTestLeafPartition->getRecord(j)->getValue(classIdx)->toString(true);
				if (!isC45) {
					if (classValue == ">50K")
						classValue = "+1 ";
					else
						classValue = "-1 ";
					transTestFile.WriteString(classValue + str + _T("\n"));
				}
				else 
					transTestFile.WriteString(str + classValue + TD_RAWDATA_TERMINATOR + _T("\n"));
			}
		}
		transTestFile.Close();
		// Finsh test file
    }

    catch (CFileException&) {
        cerr << _T("Failed to write transformed data file: ") << m_transformedDataFile << endl;
        ASSERT(false);
        return false;
    }

	cout << _T("Longest root-to-leaf path is: ") << longestPath << endl;
    cout << _T("Writing Multidimensional records succeeded.") << endl << endl;
    
	return true;
}

//---------------------------------------------------------------------------
// Converts a generalized record to a C4.5 or SVM record format
//---------------------------------------------------------------------------
void CTDDataMgr::convertRecord(CTDPartition* pLeafPartition, bool isC45, bool isTestPartition, CString& str)
{
	str.Empty();
	bool flag = false;
	CTDRecord * pRec = NULL;
	CTDContConcept* pRootContConcept = NULL;
	CTDContConcept* pCurrContConcept = NULL;
	CTDDiscConcept* pCurrDiscConcept = NULL;

	int aIdx = 0;	// Attribute index
	int cIdx = 1;	// Concept index
	CTDPartAttrib* pPartAttrib=  NULL;

	if (isTestPartition)
		pRec = pLeafPartition->getRecord(0);	// Contains at least 1 record
	else
		pRec = pLeafPartition->getGenRecords()->GetAt(0);

	// Iterate through pRec.
	// partAttribs does not contain the class attribute. Will append later.
	for (POSITION pos = pLeafPartition->getPartAttribs()->GetHeadPosition(); pos != NULL; ++aIdx) {
		pPartAttrib = pLeafPartition->getPartAttribs()->GetNext(pos);
				
		flag = false;

		// Current value belongs to a continuous attribte
		if (pPartAttrib->getActualAttrib()->isContinuous()) {
			LPTSTR tempStr = NULL;
			pCurrContConcept = static_cast <CTDContConcept*> (pRec->getValue(aIdx)->getCurrentConcept());
			if (isC45) {
				// C4.5 classifier
				// Write midpoint
				float midpoint = ((pCurrContConcept->m_upperBound + pCurrContConcept->m_lowerBound) / 2);
				tempStr = pCurrContConcept->FloatToStr(midpoint, TD_CONTVALUE_NUMDEC);
				str += tempStr;
				str += TD_RAWDATA_DELIMETER;
				delete [] tempStr;
				tempStr = NULL;
				flag = true;
				continue;
			}
			else {
				// SVM classifier
				// Write normalized interval value [0-1]
				pRootContConcept = static_cast <CTDContConcept*> (pCurrContConcept->getAttrib()->getConceptRoot());
				float normValue = (pCurrContConcept->m_upperBound - pCurrContConcept->m_lowerBound) / (pRootContConcept->m_upperBound - pRootContConcept->m_lowerBound);
				tempStr = pCurrContConcept->FloatToStr(normValue, TD_CONTVALUE_NUMDEC);
				str += pCurrContConcept->FloatToStr(cIdx);
				str += ":";
				str += tempStr;
				str += " ";
				delete [] tempStr;
				tempStr = NULL;
				cIdx += 1;
				flag = true;
				continue;
			}
		}

		CTDConcepts* pTargetConcepts = pPartAttrib->getActualAttrib()->getMultiDimConcepts();
		CTDConcept*	pTargetConcept = NULL;
		int nConcepts = pTargetConcepts->GetSize();
		pCurrDiscConcept = static_cast <CTDDiscConcept*> (pRec->getValue(aIdx)->getCurrentConcept());				
		for (int w = 0; w < nConcepts; ++w) {
			pTargetConcept = pTargetConcepts->GetAt(w);

			// Target concept is the root. Write 1
			if (pTargetConcept->m_depth == 0) {
				if (isC45) {
					str += "1";
					str += TD_RAWDATA_DELIMETER;
					flag = true;
					continue;
				}
				else {
					str += CTDContConcept::FloatToStr(cIdx + w);
					str += ":1 ";
					flag = true;
					continue;
				}
			}
					
			// Target concept is not current concept and has same or higher depth in hierarchy tree
			// Write 0
			if ((pCurrDiscConcept->m_conceptValue != pTargetConcept->m_conceptValue) && (pCurrDiscConcept->m_depth <= pTargetConcept->m_depth)) {
				if (isC45) {
					str += "0";	
					str += TD_RAWDATA_DELIMETER;
					flag = true;
					continue;
				}
				else
					continue;
			}

			// Target concept is current concept
			// Write 1
			if (pCurrDiscConcept->m_conceptValue == pTargetConcept->m_conceptValue) {
				if (isC45) {
					str += "1";
					str += TD_RAWDATA_DELIMETER;
					flag = true;
					continue;
				}
				else {
					str += CTDContConcept::FloatToStr(cIdx + w);
					str += ":1 ";
					flag = true;
					continue;
				}
			}
					
			// Target concept is an ancestor
			// Write 1, else, write 0
			if (pCurrDiscConcept->isAncestor(pTargetConcept)) {
				if (isC45) {
					str += "1";
					str += TD_RAWDATA_DELIMETER;
					flag = true;
					continue;
				}
				else {
					str += CTDContConcept::FloatToStr(cIdx + w);
					str += ":1 ";
					flag = true;
					continue;
				}
			}
			else {
				if (isC45) {
					str += "0";	
					str += TD_RAWDATA_DELIMETER;
					flag = true;
					continue;
				}
				else 
					continue;
			}

			ASSERT(flag);
		}

		cIdx += nConcepts;
	}

	return;
}

//---------------------------------------------------------------------------
// Prints out the root-to-leaf path
//---------------------------------------------------------------------------
void CTDDataMgr::printPath(CTDPartition* pLeafPartition)
{
	// Info 
	cout << "--> PartitionIdx: " << pLeafPartition->getPartitionIdx() << ". "
		 << "m_path length: " << pLeafPartition->m_path.GetSize() - 1 << ". "
		 << "# of records: " << pLeafPartition->getNumRecords() << endl;

	// Path
	cout << "Root-to-leaf path: " << endl;
	for (int q = 0; q < pLeafPartition->m_path.GetSize(); ++q) 
		cout << pLeafPartition->m_path.GetAt(q) << " ";

	cout << endl << endl;

	return;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
bool CTDDataMgr::addRecord(CTDRecord* pRecord)
{
    try {
        m_records.Add(pRecord);
        return true;
    }
    catch (CMemoryException&) {
        ASSERT(false);
        return false;
    }
}
