NTSTATUS  AbcConfigCallback(
	IN PVOID Context, IN PUNICODE_STRING PathName,
	IN INTERFACE_TYPE BusType, IN ULONG BusNumber, IN PKEY_VALUE_FULL_INFORMATION *BusInfo,
	IN CONFIGURATION_TYPE CtrlrType, IN ULONG CtrlrNumber, IN PKEY_VALUE_FULL_INFORMATION *CtrlrInfo,
	IN CONFIGURATION_TYPE PeripheralType, IN ULONG PeripheralNumber, IN PKEY_VALUE_FULL_INFORMATION *PeripheralInfo
	)
{
	DebugPrint( "ConfigCallback: Bus: %d,%d",BusType,BusNumber);
	DebugPrint( "ConfigCallback: Controller: %d,%d",CtrlrType,CtrlrNumber);
	DebugPrint( "ConfigCallback: Peripheral: %d,%d",PeripheralType,PeripheralNumber);

	if( CtrlrInfo!=NULL)
	{
		PCM_FULL_RESOURCE_DESCRIPTOR frd = (PCM_FULL_RESOURCE_DESCRIPTOR)
			(((PUCHAR)CtrlrInfo[IoQueryDeviceConfigurationData])
			+CtrlrInfo[IoQueryDeviceConfigurationData]->DataOffset);
		for( ULONG i=0; i<frd->PartialResourceList.Count; i++)
		{
			PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = &frd->PartialResourceList.PartialDescriptors[i];
			switch( resource->Type)
			{
			case CmResourceTypePort:
				DebugPrint( "ConfigCallback: I/O port %x,%d",
						resource->u.Port.Start.LowPart, resource->u.Port.Length);
				break;
			case CmResourceTypeInterrupt:
				DebugPrint( "ConfigCallback: Interrupt level %d vector %d", resource->u.Interrupt.Level, resource->u.Interrupt.Vector);
				break;
			default:
				DebugPrint( "ConfigCallback: Resource type %d",resource->Type);
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS FindParallelPort()
{
	NTSTATUS status;
	for( int BusType=0; BusType<MaximumInterfaceType; BusType++)
	{
		INTERFACE_TYPE iBusType = (INTERFACE_TYPE)BusType;
		CONFIGURATION_TYPE CtrlrType = ParallelController;
		ULONG BusNumber = 0;
		while(true)
		{
			// See if this bus instance exists
			status = IoQueryDeviceDescription(
					&iBusType, &BusNumber,
					NULL, NULL,
					NULL, NULL,
					AbcConfigCallback, NULL);
			if( !NT_SUCCESS(status))
			{
				if( status != STATUS_OBJECT_NAME_NOT_FOUND)
					return status;
				break;
			}

			// See what printers exist on this bus instance
			status = IoQueryDeviceDescription(
					&iBusType, &BusNumber,
					&CtrlrType, NULL,
					NULL, NULL,
					AbcConfigCallback, NULL);
			if( !NT_SUCCESS(status) &&
				(status != STATUS_OBJECT_NAME_NOT_FOUND))
				return status;
			BusNumber++;
		}
	}
	return status;
}
