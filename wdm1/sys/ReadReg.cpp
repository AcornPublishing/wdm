//	This code shows how to read the registry
//	However it is not used by the current Wdm1 code

void ReadReg( IN PUNICODE_STRING DriverRegistryPath)
{
	// Make zero terminated copy of driver registry path
	USHORT FromLen = DriverRegistryPath->Length;
	PUCHAR wstrDriverRegistryPath = (PUCHAR)ExAllocatePool( PagedPool, FromLen+sizeof(WCHAR));
	if( wstrDriverRegistryPath==NULL) return;
	RtlCopyMemory( wstrDriverRegistryPath, DriverRegistryPath->Buffer, FromLen);
	RtlZeroMemory( wstrDriverRegistryPath+FromLen, sizeof(WCHAR));

	// Initialise our ULONG and UNICODE_STRING values
	ULONG UlongValue = -1;
	UNICODE_STRING UnicodeString;
	UnicodeString.Buffer = NULL;
	UnicodeString.MaximumLength = 0;
	UnicodeString.Length = 0;

	// Build up our registry query table
	RTL_QUERY_REGISTRY_TABLE QueryTable[4];
	RtlZeroMemory( QueryTable, sizeof(QueryTable));

	QueryTable[0].Name  = L"Parameters";
	QueryTable[0].Flags = RTL_QUERY_REGISTRY_SUBKEY;
	QueryTable[0].EntryContext = NULL;
	QueryTable[1].Name  = L"UlongValue";
	QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
	QueryTable[1].EntryContext = &UlongValue;
	QueryTable[2].Name  = L""; // Default value
	QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
	QueryTable[2].EntryContext = &UnicodeString;

	// Issue query
	NTSTATUS status = RtlQueryRegistryValues(
					   RTL_REGISTRY_ABSOLUTE, (PWSTR)wstrDriverRegistryPath,
					   QueryTable, NULL, NULL);
	// Print results
	DebugPrint( "ReadReg %x: UlongValue %x UnicodeString %T",
				status, UlongValue, &UnicodeString);

	// Do not forget to free our buffer
	ExFreePool(wstrDriverRegistryPath);
}
