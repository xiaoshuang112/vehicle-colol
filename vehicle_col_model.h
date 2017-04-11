/****************************************************************************************
********
********   �ļ���:   vehicle_col_model.h
********   ��������: ���ٲ���ͼ���ɫ����ӿں�������
********   �汾:     V1.0
********   ����:     2017-1-20
********  
*****************************************************************************************/
#pragma  once
#ifndef   COL_TYPE_EXPORT_H
#define   COL_TYPE_EXPORT_H

#define VEHICLE_COL_MODEL_EXPORTS
#ifdef VEHICLE_COL_MODEL_EXPORTS
#define VEHICLE_COL_MODEL_API __declspec(dllexport)
#else
#define VEHICLE_COL_MODEL_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "./inc/OSAL_type.h"
#include "./inc/SDKCommon.h"
#include "./Vehiclecolor/Vehiclecolor.h"
#include "./TypeDetect/VehicleTypeDetect.h"


	typedef struct TagColTypeImage
	{
		OSAL_INT32 nSize;
		OSAL_INT32 nChannels;
		OSAL_INT32 depth;
		OSAL_INT32 step;
		OSAL_INT32 nColorMode;
		OSAL_INT32 dataOrder;
		OSAL_INT32 width;
		OSAL_INT32 height;
		OSAL_INT32 imageSize;
		OSAL_UCHAR *pData;	
	}ColTypeImage,*pColTypeImage;

//typedef enum
//{
//	Type_Truck                  = 0,//�󿨳���
//	Type_Bus                   = 1,// ��ͳ�
//	Type_Car                   = 2,// С�γ�
//	Type_MiniTruck             = 3,//С����
//	Type_MiniBus               = 4,//�����
//	Type_other                 = 5 // ����
//
//} TYPEResult;
//typedef enum  Vehicle_COLOR
//{ 
//
//	V_White                   =0, //��ɫ
//	V_Black                   =1, //��ɫ
//	V_Red                     =2,//��ɫ
//	V_Yellow                  =3,//��ɫ
//	V_Blue                    =4,//��ɫ
//	V_Green                   =5,//��ɫ
//	V_Pink                    =6,//��ɫ
//	V_Purple                  =7,//��ɫ
//	V_Gray                    =8,//��ɫ
//	V_Brown                   =9,//��ɫ
//	TypeColor_other           =10  //����
//
//}COLOR_RESULT;
	typedef struct TagTaskMain
	{
		OSAL_HANDLE  adaboost_model;    //������λ���
		OSAL_HANDLE  plate_model;       //����ʶ����     

		//��ɫ
		OSAL_HANDLE  handle_color; //bp���
		OSAL_INT32 Colordata_image[3];

		//����
		OSAL_HANDLE handle_blue;        //blue ��ɫ���
		OSAL_HANDLE handle_yellow;     //yellow ��ɫ���
		OSAL_INT32  data_image[3780];     //*hog;
		OSAL_INT32 *nMeasure;
		OSAL_INT32 *nTheta;
		OSAL_INT32 *nHist;

		OSAL_INT32   m_iTaskIndex;    //������

	}TaskMain,*pTaskMain;


	//�������ṹ��
	typedef struct TagCResult
	{

		OSAL_INT32   iVehicleType;              /*  �������� */
		OSAL_INT32   iVehicleColor;             /*  ������ɫ */

	}TCResult,*PTCResult;

	//����λ��
	typedef struct PlateRect{
		OSAL_INT32   x;             
		OSAL_INT32   y;             
		OSAL_INT32   width;         
		OSAL_INT32   height;
		OSAL_INT32   color;//1 ���ƣ�2 ���ƣ�3 ����; 0 �����복����Ϣ��
	}*pPlateRect;




VEHICLE_COL_MODEL_API SDKErrCode Vehicle_Init(OSAL_HANDLE *pHandle, int width, int height, pPlateRect pRect);
VEHICLE_COL_MODEL_API SDKErrCode Vehicle_Process(OSAL_HANDLE handle, pColTypeImage image, ImageDataType eType, pPlateRect pRect, PTCResult Result);
VEHICLE_COL_MODEL_API SDKErrCode Vehicle_Realse(OSAL_HANDLE *pHandle);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif