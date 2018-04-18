md adultDiscern

REM DiffMulti_discern <dataSetName> <bRemoveUnknownOnly> <nSpecialization> <privacyB> <nInputRecs> <nTraining>
REM Make sure dataSetName.rawdata and dataSetName.hchy exist together



DiffMulti_discern adult FALSE 25000 1 -1 45221 > .\adultDiscern\adult_25k_1.out
