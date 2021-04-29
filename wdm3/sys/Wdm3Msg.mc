;//////////////////////////////////////////////////////////////////////////////
;//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
;//	WDM Book for R&D Books, Miller Freeman Inc
;//
;//	Wdm3 example
;/////////////////////////////////////////////////////////////////////////////
;//	Wdm3Msg.mc:		Message Definition file
;/////////////////////////////////////////////////////////////////////////////
;//	Version history
;//	15-Dec-98	1.0.0	CC	creation
;/////////////////////////////////////////////////////////////////////////////

MessageIdTypedef = NTSTATUS

SeverityNames = (
	Success		  = 0x0:STATUS_SEVERITY_SUCCESS
	Informational = 0x1:STATUS_SEVERITY_INFORMATIONAL
	Warning		  = 0x2:STATUS_SEVERITY_WARNING
	Error		  = 0x3:STATUS_SEVERITY_ERROR
	)

FacilityNames = (
	System		= 0x0
	Wdm3		= 0x7:FACILITY_WDM3_ERROR_CODE
	)


MessageId=0x0001
Facility=Wdm3
Severity=Informational
SymbolicName=WDM3_MSG_LOGGING_STARTED
Language=English
Event logging enabled for Wdm3 Driver.
.

MessageId=+1
Facility=Wdm3
Severity=Informational
SymbolicName=WDM3_MESSAGE
Language=English
Message: %2.
.

