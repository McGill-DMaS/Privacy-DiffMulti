md adultNCP

REM DiffMulti_ncp <dataSetName> <bRemoveUnknownOnly> <nSpecialization> <privacyB> <nInputRecs> <nTraining>
REM Make sure dataSetName.rawdata and dataSetName.hchy exist together



DiffMulti_ncp adult FALSE 1000 1 -1 45221 > .\adultNCP\adult_1k_1.out
