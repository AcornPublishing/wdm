//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	UsbKbd example
/////////////////////////////////////////////////////////////////////////////
//	Usb.cpp:			USB access routines
/////////////////////////////////////////////////////////////////////////////
//	UsbGetPortStatus				Issue Get Port Status IOCTL
//	UsbResetPort					Reset port
//	UsbResetDevice					Reset device if port not enabled
//	UsbGetUsbInfo					Get some USB information (W2000 only)
//	UsbGetDeviceDescriptor			Get device descriptor (allocate memory for it)
//	UsbGetConfigurationDescriptors	Get specified Config and associated descriptors
//	UsbGetSpecifiedDescriptor		Get specified descriptor of given size
//	UsbSelectConfiguration			Select first config if it has HID keyboard interface
//	UsbDeselectConfiguration		Turn off device by selecting no configuration
//	UsbDoInterruptTransfer			Wait for non-zero 8 byte input report, or timeout
//	UsbGetStatuses					Get device, interface and endpoint statuses
//	UsbGetFrameInfo					Get current frame length and two frame numbers
//	UsbSendOutputReport				Send one byte as a SET_REPORT control transfer
//	UsbGetIdleRate					Get the current HID idle rate
//	CallUSBDI						Send off URB etc and wait for IOCTL to complete
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "UsbKbd.h"

/////////////////////////////////////////////////////////////////////////////

NTSTATUS CallUSBDI( IN PUSBKBD_DEVICE_EXTENSION dx, IN PVOID UrbEtc,
				    IN ULONG IoControlCode=IOCTL_INTERNAL_USB_SUBMIT_URB,
				    IN ULONG Arg2=0);

/////////////////////////////////////////////////////////////////////////////
//	UsbGetPortStatus:	Issue Get Port Status IOCTL

NTSTATUS UsbGetPortStatus( IN PUSBKBD_DEVICE_EXTENSION dx, OUT ULONG& PortStatus)
{
	DebugPrintMsg("Getting port status");
	PortStatus = 0;
	NTSTATUS status = CallUSBDI( dx, &PortStatus, IOCTL_INTERNAL_USB_GET_PORT_STATUS);
	DebugPrint( "Got port status %x", PortStatus);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbResetPort:	Reset port

NTSTATUS UsbResetPort( IN PUSBKBD_DEVICE_EXTENSION dx)
{
	DebugPrintMsg("Resetting port");
	NTSTATUS status = CallUSBDI( dx, NULL, IOCTL_INTERNAL_USB_RESET_PORT);
	DebugPrint( "Port reset %x", status);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbResetDevice:	Reset device if port not enabled

NTSTATUS UsbResetDevice( IN PUSBKBD_DEVICE_EXTENSION dx)
{
	ULONG PortStatus;

	NTSTATUS status = UsbGetPortStatus( dx, PortStatus);

	if( !NT_SUCCESS(status))
		return status;

	// Give up if device not connected
	if( !(PortStatus & USBD_PORT_CONNECTED))
		return STATUS_NO_SUCH_DEVICE;

	// Return OK if port enabled
	if( PortStatus & USBD_PORT_ENABLED)
		return status;

	// Port disabled so attempt reset
	status = UsbResetPort(dx);
	if( !NT_SUCCESS(status))
		return status;

	// See if it is now working
	status = UsbGetPortStatus( dx, PortStatus);
	if( !NT_SUCCESS(status))
		return status;
	if( !(PortStatus & USBD_PORT_CONNECTED) ||
		!(PortStatus & USBD_PORT_ENABLED))
		return STATUS_NO_SUCH_DEVICE;
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbGetUsbInfo:	Get some USB information (W2000 only)

NTSTATUS UsbGetUsbInfo( IN PUSBKBD_DEVICE_EXTENSION dx)
{
#if _WIN32_WINNT>=0x0500
	USB_BUS_NOTIFICATION BusInfo;
	DebugPrintMsg("Getting bus info");
	NTSTATUS status = CallUSBDI( dx, &BusInfo, IOCTL_INTERNAL_USB_GET_BUS_INFO);
	
	DebugPrint("Bus info: TotalBandwidth %d, ConsumedBandwidth %d and ControllerNameLength %d",
		BusInfo.TotalBandwidth, BusInfo.ConsumedBandwidth, BusInfo.ControllerNameLength);

	int len = BusInfo.ControllerNameLength+50;
	PUSB_HUB_NAME HubName = (PUSB_HUB_NAME)ExAllocatePool( NonPagedPool, len);
	RtlZeroMemory( HubName, len);
	if( HubName==NULL)
		return STATUS_INSUFFICIENT_RESOURCES;

	status = CallUSBDI( dx, HubName, IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME, BusInfo.ControllerNameLength);
	if( NT_SUCCESS(status))
		DebugPrint("Controller name is %*S", HubName->ActualLength, HubName->HubName);
	else
		DebugPrintMsg("Cannot get controller name");
	ExFreePool(HubName);
	return status;
#else
	return STATUS_NOT_SUPPORTED;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//	UsbGetDeviceDescriptor:	Get device descriptor (allocate memory for it)
//							Remember to ExFreePool this memory

NTSTATUS UsbGetDeviceDescriptor( IN PUSBKBD_DEVICE_EXTENSION dx,
								 OUT PUSB_DEVICE_DESCRIPTOR& deviceDescriptor,
								 OUT ULONG& Size)
{
	// Allocate memory for URB
	USHORT UrbSize = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
	PURB urb = (PURB)ExAllocatePool(NonPagedPool, UrbSize);
	if( urb==NULL)
	{
		DebugPrintMsg("No URB memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Allocate memory for device descriptor
	ULONG sizeDescriptor = sizeof(USB_DEVICE_DESCRIPTOR);
	deviceDescriptor = (PUSB_DEVICE_DESCRIPTOR)ExAllocatePool(NonPagedPool, sizeDescriptor);
	if( deviceDescriptor==NULL)
	{
		ExFreePool(urb);
		DebugPrintMsg("No descriptor memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Build the Get Descriptor URB
	UsbBuildGetDescriptorRequest(
		urb, UrbSize, 
		USB_DEVICE_DESCRIPTOR_TYPE, 0, 0,	// Types, Index & LanguageId
		deviceDescriptor, NULL, sizeDescriptor,    // Transfer buffer
		NULL);	// Link URB

	// Call the USB driver
	DebugPrintMsg("Getting device descriptor");
	NTSTATUS status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
	}
	// Remember count of bytes actually transferred
	Size = urb->UrbControlDescriptorRequest.TransferBufferLength;

	ExFreePool(urb);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbGetConfigurationDescriptors:	Get specified Config and associated descriptors
//									Allocate memory for descriptors.
//									Remember to ExFreePool this memory

NTSTATUS UsbGetConfigurationDescriptors( IN PUSBKBD_DEVICE_EXTENSION dx, 
										 OUT PUSB_CONFIGURATION_DESCRIPTOR& descriptors,
										 IN UCHAR ConfigIndex,
										 OUT ULONG& DescriptorsSize)
{
	// Allocate memory for URB
	USHORT UrbSize = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
	PURB urb = (PURB)ExAllocatePool(NonPagedPool, UrbSize);
	if( urb==NULL)
	{
		DebugPrintMsg("No URB memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Allocate memory just for basic config descriptor
	DescriptorsSize = sizeof(USB_CONFIGURATION_DESCRIPTOR);
	descriptors = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool, DescriptorsSize+16);
	if( descriptors==NULL)
	{
		DebugPrintMsg("No initial descriptor memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Build the Get Descriptor URB
	UsbBuildGetDescriptorRequest(
		urb, UrbSize,
		USB_CONFIGURATION_DESCRIPTOR_TYPE, ConfigIndex, 0,
		descriptors, NULL, DescriptorsSize,
		NULL);

	// Call the USB driver
	DebugPrintMsg("Getting basic configuration descriptor");
	NTSTATUS status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
		goto fail;
	}
	DebugPrint("Got basic config descr.  MaxPower %d units of 2mA", descriptors->MaxPower);

	// Reallocate memory for config descriptor and associated descriptors
	DescriptorsSize = descriptors->wTotalLength;
	ExFreePool(descriptors);
	descriptors = (PUSB_CONFIGURATION_DESCRIPTOR)ExAllocatePool(NonPagedPool, DescriptorsSize+16);
	if( descriptors==NULL)
	{
		DebugPrintMsg("No full descriptors memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Build the Get Descriptor URB
	UsbBuildGetDescriptorRequest(
		urb, UrbSize, 
		USB_CONFIGURATION_DESCRIPTOR_TYPE, ConfigIndex, 0,
		descriptors, NULL, DescriptorsSize,
		NULL);

	// Call the USB driver
	DebugPrintMsg("Getting full configuration descriptors");
	status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
		goto fail;
	}
	// Remember count of bytes actually transferred
	DescriptorsSize = urb->UrbControlDescriptorRequest.TransferBufferLength;

fail:
	ExFreePool(urb);
	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbGetSpecifiedDescriptor:	Get specified descriptor of given size
//								Allocate memory for descriptor.
//								Remember to ExFreePool this memory

NTSTATUS UsbGetSpecifiedDescriptor( IN PUSBKBD_DEVICE_EXTENSION dx,
									OUT PVOID& Descriptor,
									IN UCHAR DescriptorType,
									IN OUT ULONG& Size)
{
	// Allocate memory for URB
	USHORT UrbSize = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
	PURB urb = (PURB)ExAllocatePool(NonPagedPool, UrbSize);
	if( urb==NULL)
	{
		DebugPrintMsg("No URB memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Allocate memory for descriptor
	Descriptor = ExAllocatePool(NonPagedPool, Size);
	if( Descriptor==NULL)
	{
		ExFreePool(urb);
		DebugPrintMsg("No descriptor memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Build the Get Descriptor URB
	UsbBuildGetDescriptorRequest(
		urb, UrbSize,
		DescriptorType,	0, 0,	// Types, Index & LanguageId
		Descriptor, NULL, Size,	// Transfer buffer
		NULL);	// Link URB

	// Call the USB driver
	DebugPrint("Getting descriptor type %2x", DescriptorType);
	NTSTATUS status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
	}
	Size = urb->UrbControlDescriptorRequest.TransferBufferLength;

	ExFreePool(urb);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbSelectConfiguration:	Select first config if it has HID keyboard interface

NTSTATUS UsbSelectConfiguration( IN PUSBKBD_DEVICE_EXTENSION dx)
{
	dx->UsbPipeHandle = NULL;

	// Get all first configuration descriptors
	PUSB_CONFIGURATION_DESCRIPTOR Descriptors = NULL;
	ULONG size;
	NTSTATUS status = UsbGetConfigurationDescriptors( dx, Descriptors, 0, size);
	if( !NT_SUCCESS(status))
	{
		DebugPrint("UsbGetConfigurationDescriptors failed %x", status);
		FreeIfAllocated(Descriptors);
		return status;
	}

	// Search for an interface with HID keyboard device class
	PUSB_INTERFACE_DESCRIPTOR id = USBD_ParseConfigurationDescriptorEx(
		Descriptors, Descriptors,
		-1, -1,		// Do not search by InterfaceNumber or AlternateSetting
		3, 1, 1);	// Search for a HID device, boot protocol, keyboard
	if( id==NULL)
	{
		DebugPrintMsg("No matching interface found");
		FreeIfAllocated(Descriptors);
		return STATUS_NO_SUCH_DEVICE;
	}

	// Build list of interfaces we are interested in
	USBD_INTERFACE_LIST_ENTRY ilist[2];
	ilist[0].InterfaceDescriptor = id;
	ilist[0].Interface = NULL;	// Will point to urb->UrbUsbSelectConfiguration.Interface
	ilist[1].InterfaceDescriptor = NULL;

	// Create select configuration URB
	PURB urb = USBD_CreateConfigurationRequestEx( Descriptors, ilist);

	// Call the USB driver
	DebugPrintMsg("Selecting configuration");
	status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
	}
	else
	{
		// Select config worked
		DebugPrintMsg("Select configuration worked");
		dx->UsbConfigurationHandle = urb->UrbSelectConfiguration.ConfigurationHandle;
		// Find pipe handle of first pipe, ie interrupt pipe that returns input HID reports
		PUSBD_INTERFACE_INFORMATION InterfaceInfo = &urb->UrbSelectConfiguration.Interface;
		DebugPrint("interface Class %d NumberOfPipes %d", InterfaceInfo->Class, InterfaceInfo->NumberOfPipes);
		if( InterfaceInfo->NumberOfPipes>0)
		{
			PUSBD_PIPE_INFORMATION pi = &InterfaceInfo->Pipes[0];
			dx->UsbPipeHandle = pi->PipeHandle;
			DebugPrint("PipeHandle = %x", dx->UsbPipeHandle);
			DebugPrint("Pipes[0] EndpointAddress %2x Interval %dms PipeType %d MaximumTransferSize %d",
				pi->EndpointAddress, pi->Interval, pi->PipeType, pi->MaximumTransferSize);
		}
		if( dx->UsbPipeHandle==NULL)
			status = STATUS_UNSUCCESSFUL;
	}

	FreeIfAllocated(urb);
	FreeIfAllocated(Descriptors);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbDeselectConfiguration:	Turn off device by selecting no configuration

NTSTATUS UsbDeselectConfiguration( IN PUSBKBD_DEVICE_EXTENSION dx)
{
	dx->UsbPipeHandle = NULL;

	// Allocate memory for URB
	USHORT UrbSize = sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST);
	PURB urb = (PURB)ExAllocatePool(NonPagedPool, UrbSize);
	if( urb==NULL)
	{
		DebugPrintMsg("No URB memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Build select configuration URB with NULL Config descriptor
	UsbBuildSelectConfigurationRequest( urb, UrbSize, NULL);

	// Call the USB driver
	DebugPrintMsg("Deselecting configuration");
	NTSTATUS status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
	}

	ExFreePool(urb);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbDoInterruptTransfer:	Wait for non-zero 8 byte input report, or timeout

NTSTATUS UsbDoInterruptTransfer( IN PUSBKBD_DEVICE_EXTENSION dx, IN PVOID UserBuffer, ULONG& UserBufferSize)
{
	// Check we're selected
	if( dx->UsbPipeHandle==NULL)
		return STATUS_INVALID_HANDLE;

	// Check input parameters
	ULONG InputBufferSize = UserBufferSize;
	UserBufferSize = 0;
	if( UserBuffer==NULL || InputBufferSize<8)
		return STATUS_INVALID_PARAMETER;

	// Keyboard input reports are always 8 bytes long
	NTSTATUS status = STATUS_SUCCESS;
	ULONG OutputBufferSize = 8;

	// Allocate memory for URB
	USHORT UrbSize = sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER);
	PURB urb = (PURB)ExAllocatePool(NonPagedPool, UrbSize);
	if( urb==NULL)
	{
		DebugPrintMsg("No URB memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Remember when we started
	// Get start tick count and length of tick in 100ns units
	LARGE_INTEGER StartTickCount;
	KeQueryTickCount( &StartTickCount);
	ULONG UnitsOf100ns = KeQueryTimeIncrement();
//	DebugPrint("Time increment %d", UnitsOf100ns);

	// Loop until non-zero report read, error, bad length, or timed out
//	DebugPrintMsg("Reading Interrupt data");
	while( true)
	{
		// Build Do Bulk or Interrupt transfer request
		UsbBuildInterruptOrBulkTransferRequest(
			urb, UrbSize,
			dx->UsbPipeHandle,
			UserBuffer, NULL, OutputBufferSize,
			USBD_TRANSFER_DIRECTION_IN,
			NULL);

		// Call the USB driver
		status = CallUSBDI( dx, urb);
		// Check statuses
		if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
		{
			DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
			status = STATUS_UNSUCCESSFUL;
			break;
		}

		// Give up if count of bytes transferred was not 8
		if( urb->UrbBulkOrInterruptTransfer.TransferBufferLength!=OutputBufferSize)
			break;

		// If data non-zero then exit as we have a keypress
		__int64* pData = (__int64 *)UserBuffer;
		if( *pData!=0i64)
		{
//			DebugPrint("Got some data");
			break;
		}

		// Check for timeout
		LARGE_INTEGER TickCountNow;
		KeQueryTickCount( &TickCountNow);
		ULONG ticks = (ULONG)(TickCountNow.QuadPart - StartTickCount.QuadPart);
		if( ticks*UnitsOf100ns/10000000 >= dx->UsbTimeout)
		{
			DebugPrint("Timeout %d 100ns", ticks*UnitsOf100ns);
			status = STATUS_NO_MEDIA_IN_DEVICE;
			break;
		}
	}
	UserBufferSize = urb->UrbBulkOrInterruptTransfer.TransferBufferLength;

//	DebugPrint("Transfer length %d", urb->UrbBulkOrInterruptTransfer.TransferBufferLength);
	if( NT_SUCCESS(status))
	{
		PUCHAR bd = (PUCHAR)UserBuffer;
		DebugPrint("Transfer data %2x %2x %2x %2x %2x %2x %2x %2x",
			bd[0], bd[1], bd[2], bd[3], bd[4], bd[5], bd[6], bd[7]);
	}

	ExFreePool(urb);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbGetStatuses:	Get device, interface and endpoint statuses

NTSTATUS UsbGetStatuses( IN PUSBKBD_DEVICE_EXTENSION dx,
						 OUT PUCHAR Statuses,
						 IN ULONG Size)
{
	// Input buffer must have 2 bytes for each status
	if( Size!=6)
		return STATUS_INVALID_PARAMETER;

	// Allocate memory for URB
	USHORT UrbSize = sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST);
	PURB urb = (PURB)ExAllocatePool(NonPagedPool, UrbSize);
	if( urb==NULL)
	{
		DebugPrintMsg("No URB memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	/////////////////////////////////////////////////////////////////////////
	// Build URB to get device status
	UsbBuildGetStatusRequest(
		urb,
		URB_FUNCTION_GET_STATUS_FROM_DEVICE, 0,		// Op and Index
		Statuses, NULL,    // Transfer buffer
		NULL);	// Link URB

	// Call the USB driver
	DebugPrintMsg("Getting device status");
	NTSTATUS status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
		goto fail;
	}
	Size = urb->UrbControlGetStatusRequest.TransferBufferLength;
	if( Size!=2) goto fail;

	/////////////////////////////////////////////////////////////////////////
	// Build URB to get interface status
	UsbBuildGetStatusRequest(
		urb,
		URB_FUNCTION_GET_STATUS_FROM_INTERFACE, 0,		// Op and Index
		Statuses+2, NULL,    // Transfer buffer
		NULL);	// Link URB

	// Call the USB driver
	DebugPrintMsg("Getting interface status");
	status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
		goto fail;
	}
	Size = urb->UrbControlGetStatusRequest.TransferBufferLength;
	if( Size!=2) goto fail;

	/////////////////////////////////////////////////////////////////////////
	// Build URB to get endpoint status
	UsbBuildGetStatusRequest(
		urb,
		URB_FUNCTION_GET_STATUS_FROM_ENDPOINT, 0,		// Op and Index
		Statuses+4, NULL,    // Transfer buffer
		NULL);	// Link URB

	// Call the USB driver
	DebugPrintMsg("Getting endpoint status");
	status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
		goto fail;
	}
	Size = urb->UrbControlGetStatusRequest.TransferBufferLength;
	if( Size!=2) goto fail;

	/////////////////////////////////////////////////////////////////////////
fail:
	ExFreePool(urb);
	return status;
}


/////////////////////////////////////////////////////////////////////////////
//	UsbGetFrameInfo:	Get current frame length and two frame numbers

NTSTATUS UsbGetFrameInfo( IN PUSBKBD_DEVICE_EXTENSION dx,
						  OUT ULONG& FrameLength,
						  OUT ULONG& FrameNumber,
						  OUT ULONG& FrameAlterNumber)
{
	// Allocate memory for URB
	USHORT UrbSize = sizeof(struct _URB_GET_FRAME_LENGTH);
	if( sizeof(struct _URB_GET_CURRENT_FRAME_NUMBER)>UrbSize)
		UrbSize = sizeof(struct _URB_GET_CURRENT_FRAME_NUMBER);
	PURB urb = (PURB)ExAllocatePool(NonPagedPool, UrbSize);
	if( urb==NULL)
	{
		DebugPrintMsg("No URB memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	/////////////////////////////////////////////////////////////////////////
	// Get current frame number
	// Build URB by hand
	urb->UrbHeader.Length = UrbSize;
	urb->UrbHeader.Function = URB_FUNCTION_GET_CURRENT_FRAME_NUMBER;

	// Call the USB driver
	DebugPrintMsg("Getting current frame number");
	NTSTATUS status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
		goto fail;
	}
	FrameNumber = urb->UrbGetCurrentFrameNumber.FrameNumber;
	DebugPrint("FrameNumber %d", FrameNumber);

	/////////////////////////////////////////////////////////////////////////
	// Get current frame length and frame number when length can be altered
	// Build URB by hand
	urb->UrbHeader.Length = UrbSize;
	urb->UrbHeader.Function = URB_FUNCTION_GET_FRAME_LENGTH;

	// Call the USB driver
	DebugPrintMsg("Getting frame info");
	status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
	}

	// Store info
	FrameLength = urb->UrbGetFrameLength.FrameLength;
	FrameAlterNumber = urb->UrbGetFrameLength.FrameNumber;
	DebugPrint("FrameLength %d FrameAlterNumber %d", FrameLength, FrameAlterNumber);
fail:
	ExFreePool(urb);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbSendOutputReport:	Send one byte as a SET_REPORT control transfer

const UCHAR SET_REPORT = 0x09;

NTSTATUS UsbSendOutputReport( IN PUSBKBD_DEVICE_EXTENSION dx, IN UCHAR OutputData)
{
	// Allocate memory for URB
	USHORT UrbSize = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
	PURB urb = (PURB)ExAllocatePool(NonPagedPool, UrbSize);
	if( urb==NULL)
	{
		DebugPrintMsg("No URB memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Build URB to send Class interface control request on Default pipe
	UsbBuildVendorRequest(urb,
		URB_FUNCTION_CLASS_INTERFACE, UrbSize,
		USBD_TRANSFER_DIRECTION_OUT,	// Direction out
		0,			// Reserved bits
		SET_REPORT,	// Request
		0x0200,		// Output report type, Report id zero
		0,			// interface index
		&OutputData, NULL, 1,	// Output data
		NULL);

	// Call the USB driver
	DebugPrintMsg("Sending set report");
	NTSTATUS status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
	}
	ExFreePool(urb);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbGetIdleRate:	Get the current HID idle rate

const UCHAR GET_IDLE = 0x02;

NTSTATUS UsbGetIdleRate( IN PUSBKBD_DEVICE_EXTENSION dx)
{
	// Allocate memory for URB
	USHORT UrbSize = sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST);
	PURB urb = (PURB)ExAllocatePool(NonPagedPool, UrbSize);
	if( urb==NULL)
	{
		DebugPrintMsg("No URB memory");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	// Build URB to send Class interface control request on Default pipe
	UCHAR IdleRate = 0;
	UsbBuildVendorRequest(urb,
		URB_FUNCTION_CLASS_INTERFACE, UrbSize,
		USBD_TRANSFER_DIRECTION_IN,		// Direction in
		0,			// Reserved bits
		GET_IDLE,	// Request
		0x0000,		// No report type, Report id zero
		0,			// interface index
		&IdleRate, NULL, 1,	// Output data
		NULL);

	// Call the USB driver
	DebugPrintMsg("Sending Get Idle request");
	NTSTATUS status = CallUSBDI( dx, urb);
	// Check statuses
	if( !NT_SUCCESS(status) || !USBD_SUCCESS( urb->UrbHeader.Status))
	{
		DebugPrint("status %x URB status %x", status, urb->UrbHeader.Status);
		status = STATUS_UNSUCCESSFUL;
	}
	DebugPrint( "Idle rate is %d units of 4ms", IdleRate);
	ExFreePool(urb);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	CallUSBDI:	Send off URB etc and wait for IOCTL to complete
//				Build Internal IOCTL IRP to send to USBDI
//				Call USBDI and wait for IRP to complete
//				Must be called at PASSIVE_LEVEL

NTSTATUS CallUSBDI( IN PUSBKBD_DEVICE_EXTENSION dx, IN PVOID UrbEtc,
					IN ULONG IoControlCode/*=IOCTL_INTERNAL_USB_SUBMIT_URB*/,
				    IN ULONG Arg2/*=0*/)
{
	IO_STATUS_BLOCK IoStatus;
	KEVENT event;

	// Initialise IRP completion event
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	// Build Internal IOCTL IRP
	PIRP Irp = IoBuildDeviceIoControlRequest(
					IoControlCode, dx->NextStackDevice,
					NULL, 0,	// Input buffer
					NULL, 0,	// Output buffer
					TRUE, &event, &IoStatus);

	// Get IRP stack location for next driver down (already set up)
	PIO_STACK_LOCATION NextIrpStack = IoGetNextIrpStackLocation(Irp);
	// Store pointer to the URB etc
	NextIrpStack->Parameters.Others.Argument1 = UrbEtc;
	NextIrpStack->Parameters.Others.Argument2 = (PVOID)Arg2;

	// Call the driver and wait for completion if necessary
	NTSTATUS status = IoCallDriver( dx->NextStackDevice, Irp);
	if (status == STATUS_PENDING)
	{
//		DebugPrintMsg("CallUSBDI: waiting for URB completion");
		status = KeWaitForSingleObject( &event, Suspended, KernelMode, FALSE, NULL);
		status = IoStatus.Status;
	}

	// return IRP completion status
//	DebugPrint("CallUSBDI returned %x",status);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
