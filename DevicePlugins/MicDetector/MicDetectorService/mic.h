//----------------------------------------------------------------------------
//                        General MIC control interface.
//                               Author V.Porosev.
//                             Copyright (c) 2000-2010
//
//----------------------------------------------------------------------------
//This library provides software interface with MultiChannel Ionisation Chamber.
//----------------------------------------------------------------------------
#ifndef __MIC_H
#define __MIC_H

#include <windows.h>

#ifdef  _MIC_API
#undef  _MIC_API
#endif
#ifdef  _MIC_DLL   // Defined When Building DLL
#define _MIC_API   __declspec( dllexport )
#else
#define _MIC_API   __declspec( dllimport )
#endif

//error codes
#define  MIC_SUCCESS             0x0000
#define  MIC_ERROR_INIT          0x1001
#define  MIC_ERROR_TIMEOUT       0x1002
#define  MIC_ERROR_PARAMETERS    0x1003
#define  MIC_ERROR_RESPONSE      0x1004

//mic option flags 
#define  MIC_OPTION_NONE         0x0000    // nothing installed
#define  MIC_OPTION_PORT485      0x0001    // port485 present
#define  MIC_OPTION_OPTICAL      0x0002    // optical control port present
#define  MIC_OPTION_MEMORY       0x0004    // buffer memory installed
     
//hardware configuration
# pragma pack(push,1)
typedef struct 
{
   char  Signature[128];  ///Library description
   WORD  MaxXdim;         ///Max width of data
   WORD  MaxYdim;         ///Max length of data
   WORD  FonYdim;         ///Y dimention of background data
   WORD  FonTopOffset;    ///Top offset to calculate data
   WORD  FonBottomOffset; ///Bottom offset to calculate data
   WORD  MaxValue;        ///Max value of data
   WORD  Options;         ///Optional devices configuration
   WORD  NumModules;      ///Number of elecronics modules
   WORD  NumChannels;     ///Number channels in each modules
   WORD  NumVoltages;     ///Number of controlled voltage units
   float fADCBitValue;    ///ADC Bit value (Volts)
   float fInternalGain;   ///Internal electronics gain before ADC
}HARDWARE_INFO;
# pragma pack(pop)

#if defined __cplusplus
extern "C"
{
#endif

#ifdef _MSC_VER
#define ReadHardwareInfo                   _ReadHardwareInfo
#define Hardware_MakeRequest               _Hardware_MakeRequest
#define HardwareInit                       _HardwareInit                     
#define HardwareShutdown                   _HardwareShutdown                  
#define Hardware_ActivatePrimaryAdapter    _Hardware_ActivatePrimaryAdapter  
#define Hardware_ActivateSecondaryAdapter  _Hardware_ActivateSecondaryAdapter
#define PrepareReading                     _PrepareReading                   
#define ReadData                           _ReadData                         
#define CloseReading                       _CloseReading                     
#define ReadFonData                        _ReadFonData                      
#define GetVoltage                         _GetVoltage                       
#define SetGeneratorMode                   _SetGeneratorMode                 
#define Monitor_ReadData                   _Monitor_ReadData                 
#define Memory_PrepareWrite                _Memory_PrepareWrite              
#define Memory_Write                       _Memory_Write                     
#define Memory_Read                        _Memory_Read                       
#define Memory_GetSize                     _Memory_GetSize                   
#define ReadMemoryData                     _ReadMemoryData                    
#define ReadUnprocessedData                _ReadUnprocessedData              
#define ProcessDataLine                    _ProcessDataLine                  
#define RS485_Send                         _RS485_Send                       
#define RS485_Receive                      _RS485_Receive                    
#define Optical_Send                       _Optical_Send                     
#define Optical_Receive                    _Optical_Receive                  
#define GetOffsets                         _GetOffsets                       
#define Hardware_SetUserFunction           _Hardware_SetUserFunction         
#define Hardware_ResetUserFunction         _Hardware_ResetUserFunction       
#define Hardware_SetPreviewFunction        _Hardware_SetPreviewFunction
#define Hardware_ResetPreviewFunction      _Hardware_ResetPreviewFunction
#endif

#ifdef _MIC_DLL 
#include "tcharhlp.icc"
#endif

//---------------------------------------------------------------------------
// Read DLL verion and internal hardware parameters
_MIC_API void ReadHardwareInfo(HARDWARE_INFO* version);

//---------------------------------------------------------------------------
// For future versions prototyping. Not realised yet!
//---------------------------------------------------------------------------
//
// Object Identifiers used by Request Query/Set Information
//

#define MIC_SUPPORTED_LIST                  0x00010001
#define MIC_GEN_MAXXDIM                     0x00010002
#define MIC_GEN_MAXYDIM                     0x00010003
#define MIC_GEN_MAXDATAVALUE                0x00010004
#define MIC_GEN_OPTIONS                     0x00010005
#define MIC_GEN_MODULES                     0x00010006
#define MIC_GEN_CHANNELS                    0x00010007
#define MIC_GEN_VOLTAGES                    0x00010008
#define MIC_ADC_BITVALUE                    0x00020001
#define MIC_ADC_DATAGAIN                    0x00020002

//etc...

typedef enum _MIC_REQUEST_TYPE
{
    MICRequestQueryInformation,
    MICRequestSetInformation
} MIC_REQUEST_TYPE, *PMIC_REQUEST_TYPE;


typedef struct _MIC_REQUEST
{
   MIC_REQUEST_TYPE RequestType;
   union _MIC_DATA
   {
      struct _MIC_QUERY_INFORMATION
      {
         UINT    Oid;
         PVOID   InformationBuffer;
         UINT    InformationBufferLength;
         UINT    BytesWritten;
         UINT    BytesNeeded;
      }
      QUERY_INFORMATION;

      struct _MIC_SET_INFORMATION
      {
         UINT    Oid;
         PVOID   InformationBuffer;
         UINT    InformationBufferLength;
         UINT    BytesRead;
         UINT    BytesNeeded;
      }
      SET_INFORMATION;

   }DATA;
} MIC_REQUEST, *PMIC_REQUEST;


//Make request to hardware
_MIC_API WORD Hardware_MakeRequest( PMIC_REQUEST pMICRequest );


//---------------------------------------------------------------------------
// Hardware initialisation AND termination
// inifile - param. is not used!
// opcode  - param. is not used!
// On Succes this function returns      - MIC_SUCCESS
// On memory allocation error           - MIC_ERROR_INIT
// On wrong Regisry info or user rights - MIC_ERROR_PARAMETERS
// On wrong connection with detector    - MIC_ERROR_RESPONSE
_MIC_API WORD  HardwareInit( char* inifile, WORD* opcode );

_MIC_API void  HardwareShutdown();

//---------------------------------------------------------------------------
// Two adapter manipulators.
// If Adapter is not correctly defined in regestry - MIC_ERROR_PARAMETERS
// If Adapter is not correctly connected to device - MIC_ERROR_RESPONSE
// Otherwise - MIC_SUCCESS

//Activate Primary adapter.
_MIC_API WORD  Hardware_ActivatePrimaryAdapter();

//Activate Secondary adapter
_MIC_API WORD  Hardware_ActivateSecondaryAdapter();

//---------------------------------------------------------------------------
//Prepare data measurement
//fTime - charge collection time (Value in ms depends on electronics)
//Xdim  - X image dimention
//Ydim  - Y image dimention
_MIC_API bool  PrepareReading(float fTime, WORD Xdim, WORD Ydim);
//on return - status of operation

//---------------------------------------------------------------------------
//Read data from detector
//on return - actual number of measured lines
//Array     - pointer to external data array
//Xdim      - X image dimention
//Ydim      - Y image dimention
_MIC_API WORD  ReadData(WORD*  Array, WORD Xdim, WORD Ydim);
//on return - actual number of measured lines

//---------------------------------------------------------------------------
//Post read operation
//Array     - pointer to external data array
//Xdim      - X image dimention
//Ydim      - Y image dimention
//bRemove   - remove background
_MIC_API bool  CloseReading(WORD*  Array, WORD Xdim, WORD Ydim, bool bRemove = false);
//on return - status of operation

//---------------------------------------------------------------------------
//Read fon array. For hardware tests only.
_MIC_API bool  ReadFonData(WORD*  Array, WORD Xdim, WORD Ydim);
//on return - status of operation

//---------------------------------------------------------------------------
// Measure internal voltage from power source
// cont - Valid contact number
// data - buffer for measured value (mV)
_MIC_API WORD  GetVoltage(int cont, WORD* data);

//---------------------------------------------------------------------------
// Set test generator modes
#define  TEST_NONE          0
#define  TEST_INFINITY      1
#define  TEST_SAW           2
#define  TEST_STEP          3
_MIC_API WORD  SetGeneratorMode( WORD Mode );


//---------------------------------------------------------------------------
// Read Monitor data where available
_MIC_API bool  Monitor_ReadData(WORD*  Array, WORD Ydim, int nMode);
//on return - status of operation

//---------------------------------------------------------------------------
//
//                               Memory
//
//---------------------------------------------------------------------------
//prepare test write to memory
_MIC_API void  Memory_PrepareWrite(void);

//write data array to memory
_MIC_API void  Memory_Write(WORD* Data, int Length);

//Read data array from memory
_MIC_API bool  Memory_Read(WORD* Data, int Length);

//Get size of available memory on a board in bytes
_MIC_API DWORD Memory_GetSize(void);

//Read direct data from memory. For hardware tests only.
_MIC_API bool  ReadMemoryData(WORD*  Array, WORD Xdim, WORD Ydim);
//on return - status of operation

//Read draft detector data. For hardware tests only.
_MIC_API bool  ReadUnprocessedData(WORD*  Array, WORD Xdim, WORD Ydim, bool bShift = true);
//on return - status of operation

//Process draft detector data. For inner aims only.
_MIC_API void  ProcessDataLine(WORD* wData, bool bShift, bool bRemove, int nOffset, bool bMask );

//---------------------------------------------------------------------------
//
//                          Port 485 Interface
//
//---------------------------------------------------------------------------
//Send data to RS485 : num - data length
//Return: result of operation
_MIC_API bool RS485_Send(BYTE* Data, int num);

//Receive data from RS485 : maxnum - buffer length
//Return: number received bytes
_MIC_API int  RS485_Receive(BYTE* Data, int maxnum);


//---------------------------------------------------------------------------
//
//                         Optical port Interface
//
//---------------------------------------------------------------------------
//Send data to optical coupled port. 
_MIC_API bool Optical_Send(BYTE Data);

//receive status byte. 
_MIC_API bool Optical_Receive(BYTE* Data);


//---------------------------------------------------------------------------
//
//                               Service
//
//---------------------------------------------------------------------------
//internal ADC reference data
struct OFFSETS
{
   int    nADCZero    ;  
   int    nVirtualZero;  
   int    nFinalOffset;  
};

_MIC_API void GetOffsets(OFFSETS* data);

//---------------------------------------------------------------------------
//Setup external event processor.
//user function prototype that is called to abrupt Wait cycles
//to abrupt user should return TRUE, to continue FALSE
typedef  bool (__stdcall *HARDUSERACTION_FUNC)(void*);

_MIC_API void  Hardware_SetUserFunction(HARDUSERACTION_FUNC func, void* arg);
_MIC_API void  Hardware_ResetUserFunction(void);

//---------------------------------------------------------------------------
// Setup external preview window
// This function is called with 1st parameter defined at SET and 2nd equals
// to currently available number of data lines
typedef  void (__stdcall *HARDPREVIEW_FUNC)(void* arg, int);
// While set, you define pointer to WORD image with dimentions equals current image size
// wrong data are ignored.
_MIC_API void  Hardware_SetPreviewFunction(HARDPREVIEW_FUNC func, void* arg, unsigned short* bimage);
_MIC_API void  Hardware_ResetPreviewFunction(void);

#if defined __cplusplus
}
#endif

#endif

