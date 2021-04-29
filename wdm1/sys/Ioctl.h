//	DeviceIoControl IOCTL codes supported by Wdm1

#define IOCTL_WDM1_ZERO_BUFFER CTL_CODE(	\
			FILE_DEVICE_UNKNOWN,			\
			0x801,							\
			METHOD_BUFFERED,				\
			FILE_ANY_ACCESS)

#define IOCTL_WDM1_REMOVE_BUFFER CTL_CODE(	\
			FILE_DEVICE_UNKNOWN,			\
			0x802,							\
			METHOD_BUFFERED,				\
			FILE_ANY_ACCESS)

#define IOCTL_WDM1_GET_BUFFER_SIZE CTL_CODE(	\
			FILE_DEVICE_UNKNOWN,			\
			0x803,							\
			METHOD_BUFFERED,				\
			FILE_ANY_ACCESS)

#define IOCTL_WDM1_GET_BUFFER CTL_CODE(	\
			FILE_DEVICE_UNKNOWN,			\
			0x804,							\
			METHOD_BUFFERED,				\
			FILE_ANY_ACCESS)

#define IOCTL_WDM1_UNRECOGNISED CTL_CODE(	\
			FILE_DEVICE_UNKNOWN,			\
			0x805,							\
			METHOD_BUFFERED,				\
			FILE_ANY_ACCESS)

