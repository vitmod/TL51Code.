/*
 * Copyright Unitend Technologies Inc. 
 * 
 * This file and the information contained herein are the subject of copyright
 * and intellectual property rights under international convention. All rights
 * reserved. No part of this file may be reproduced, stored in a retrieval
 * system or transmitted in any form by any means, electronic, mechanical or
 * optical, in whole or in part, without the prior written permission of Unitend 
 * Technologies Inc.
 *
 * File name: unitend.h,
 * Author: baihuisheng
 * Version:0.0.0.1
 * Date:2009-07-17
 * Description:this file define basic data type for UTI
 * History:
 *         Date:2009-07-17    Author:baihuisheng    Modification:Creation
 */
 
#ifndef _UNITEND_H_
#define _UNITEND_H_

#define IN
#define OUT
#define INOUT

#define K   (1024)
#define M   (1024*1024)

#ifndef NULL
#define NULL 0
#endif

/************************************************************/
/*                                                          */
/*  below is basic DATA type define for UNITEND co.ltd           */
/*                                                          */
/************************************************************/

typedef unsigned char            UTI_BYTE;    /*  range :  0 to 255                   */
typedef signed char              UTI_CHAR;    /*  range :  0 to 255 or -128 to 127    */
typedef signed long              UTI_LONG;    /*  range :  -2147483648 to 2147483647  */
typedef unsigned long            UTI_ULONG;   /*  range :  0 to 4294967295            */
typedef unsigned short           UTI_WORD;    /*  range :  0 to 65535                 */
typedef unsigned long            UTI_DWORD;	 /*  range :  0 to 4294967295            */

typedef unsigned char            UTI_UINT8;   /*  range :  0 to 255                   */
typedef signed char              UTI_SINT8;	 /*  range :  0 to 255 or -128 to 127    */

typedef unsigned short           UTI_UINT16;  /*  range :  0 to 65535                 */
typedef signed short             UTI_SINT16;  /*  range :  -32767 to 32767            */

typedef unsigned long            UTI_UINT32;   /*  range :  0 to 4294967295            */
typedef signed long              UTI_SINT32;  /*  range :  -2147483648 to 2147483647  */

typedef signed int               UTI_SINT;
typedef int                      UTI_INT;
typedef unsigned int             UTI_UINT;

typedef unsigned char            UTI_BOOL;    /*  range :  TRUE or FALSE           */
typedef void                     UTI_VOID;    /*  range :  n.a.                    */

typedef UTI_BYTE                 *UTI_PBYTE;
typedef UTI_CHAR                 *UTI_PCHAR;
typedef UTI_LONG                 *UTI_PLONG;
typedef UTI_ULONG                *UTI_PULONG;
typedef UTI_WORD                 *UTI_PWORD;
typedef UTI_DWORD                *UTI_PDWORD;

typedef UTI_UINT8                *UTI_PUINT8;
typedef UTI_SINT8                *UTI_PSINT8;

typedef UTI_UINT16               *UTI_PUINT16;
typedef UTI_SINT16               *UTI_PSINT16;

typedef UTI_UINT32               *UTI_PUINT32;
typedef UTI_SINT32               *UTI_PSINT32;

typedef UTI_INT                  *UTI_PINT;
typedef UTI_SINT                 *UTI_PSINT;

typedef UTI_BOOL                 *UTI_PBOOL;
typedef UTI_VOID                 *UTI_PVOID;

#ifndef UTI_TRUE
#define UTI_TRUE (UTI_BOOL) (1)
#endif

#ifndef UTI_FALSE
#define UTI_FALSE (UTI_BOOL) (!UTI_TRUE)
#endif

#ifndef UTI_NULL
#define UTI_NULL (UTI_PVOID)(0)
#endif



#define UDRM_LOG_OFF                               0
#define UDRM_LOG_CONSOLE                           1
#define UDRM_LOG_FILE                              2
#define UDRM_LOG_CONSOLE_FILE                      3

#define UDRM_LOG_ERROR                             0
#define UDRM_LOG_WARN                              1
#define UDRM_LOG_INFO                              2
#define UDRM_LOG_DEBUG                             3


/************************************************************************/
/*									*/
/*below is some common error for unitend software			*/
/*									*/
/*									*/
/************************************************************************/

typedef UTI_UINT32               UTI_UDRM_HANDLE;
typedef UTI_INT                  UTI_RESULT;


#define UDRM_ERROR_OK                                    0      // Success                                    �ɹ�
#define UDRM_ERROR_UNKNOWN_ERROR                         -1	    // Unknown error                              δ֪����
#define UDRM_ERROR_ENCRYPT_FAILED                        -2	    // Encrypt failed                             ���ܴ���
#define UDRM_ERROR_DECRYPT_FAILED                        -3	    // Decrypt failed                             ���ܴ���
#define UDRM_ERROR_WAIT_TIMEOUT                          -4	    // Wait time out                              �ȴ���ʱ
#define UDRM_ERROR_INVALID_ARGUMENT                      -5	    // Argument is invalid                        ��������
#define UDRM_ERROR_NOT_ENOUGH_MEMORY                     -6	    // Memory is not enough                       ���ݲ���
#define UDRM_ERROR_NOT_ENOUGH_RESOURCE                   -7	    // Resource is not enough                     ��Դ����
#define UDRM_ERROR_NOT_SUPPORT                           -8	    // Not support                                ���ܲ�֧��
#define UDRM_ERROR_DEVICE_NOT_PRESENT                    -9	    // Device is not present                      �豸������
#define UDRM_ERROR_NOT_ENOUGH_BUFFER                     -10    // Buffer is not enough                       ���岻��
#define UDRM_ERROR_CONNNECT_FAILED                       -11    // Connect server failed                      ����ʧ��
#define UDRM_ERROR_UDRMCLIENT_NOT_EXIST                  -12    // DRM Client not exist                       �豸δ֪
#define UDRM_ERROR_CONTENT_NOT_EXIST                     -13    // Content not exist                          ���ݲ�����
#define UDRM_ERROR_CREDIT_NOT_ENOUGH                     -14    // Credit not enough                          �ʽ���
#define UDRM_ERROR_SYSTEM_BUSY                           -15    // System is busy                             ϵͳ��æ
#define UDRM_ERROR_DEVICE_INVALID                        -16    // Device is invalid                          �豸��Ч
#define UDRM_ERROR_HMAC_ERROR                            -17    // HMAC error                                 HMAC����
#define UDRM_ERROR_DICTATE_INVALID                       -18    // Dictate is invalid                         ָ�����
#define UDRM_ERROR_SYSTEM_ERROR                          -19    // System error                               ϵͳ����
#define UDRM_ERROR_TYPE_ERROR                            -20    // Type is error                              ���ʹ���
#define UDRM_ERROR_MESSAGE_ERROR                         -21    // Message is error                           ��Ϣ����
#define UDRM_ERROR_INDEX_ERROR                           -22    // Index is error                             ��������
#define UDRM_ERROR_DEVICE_CERT_EXIST                     -23    // Device cert exist                          �豸֤�����
#define UDRM_ERROR_DEVICE_CERT_NOT_MATCH                 -24    // Device cert not match                      �豸֤�鲻ƥ��
#define UDRM_ERROR_CERT_OK                               -25    // Cert is OK                                 ֤������
#define UDRM_ERROR_CERT_NOT_EXIST                        -26    // Cert is not exist                          ֤�鲻����
#define UDRM_ERROR_CERT_NOT_ISSUE                        -27    // Cert is not issue                          ֤��δ�䷢
#define UDRM_ERROR_CERT_SUSPEND                          -28    // Cert is suspend                            ֤����ͣ��
#define UDRM_ERROR_CERT_FORBID                           -29    // Cert is forbid                             ֤���ѽ���
#define UDRM_ERROR_CERT_UPDATE                           -30    // Cert need update                           ֤�������
#define UDRM_ERROR_CERT_REVOKE                           -31    // Cert is revoke                             ֤���ѳ���
#define UDRM_ERROR_CERT_EXPIRATION                       -32    // Cert is expiration                         ֤���ѹ���
#define UDRM_ERROR_CERT_VERIFY_ERROR                     -33    // Cert varify error                          ֤����֤����
#define UDRM_ERROR_MANUAL_SETUP                          -34    // Manual setup                               �ֶ�����
#define UDRM_ERROR_LICENSE_NOT_EXIST                     -35    // License is not exist                       ���֤������
#define UDRM_ERROR_LICENSE_INVALID                       -36    // License is invalid                         ���֤��Ч
#define UDRM_ERROR_LICENSE_UPDATE                        -37    // License need update                        ���֤�����
#define UDRM_ERROR_LICENSE_EXPIRATION                    -38    // License is expiration                      ���֤�ѹ���
#define UDRM_ERROR_CERTREQ_HAVE_NO_DEAL                  -39    // Cert request exist no deal                 ֤���������ύ
#define UDRM_ERROR_DB_CONNECT_ERROR                      -40    // Database connect error                     ���ݿ������쳣
#define UDRM_ERROR_DB_OPERATE_ERROR                      -41    // Database operate error                     ���ݿ�����쳣
#define UDRM_ERROR_ROOT_CERT_ERROR                       -42    // Root ca error                              ��֤�����
#define UDRM_ERROR_ROOT_PKEY_ERROR                       -43    // Root private key error                     ��˽Կ����
#define UDRM_ERROR_GEN_CERT_ERROR                        -44    // Cert genereate error                       ֤�������쳣
#define UDRM_ERROR_MALLOC_ERROR                          -45    // Malloc or new error                        Malloc��New�쳣
#define UDRM_ERROR_DEVICE_OPERATE_ERROR                  -46    // Device operate error                       �豸�����쳣
#define UDRM_ERROR_MY_CERT_ERROR                         -47    // Myself cert error                          �ҵ�֤���쳣
#define UDRM_ERROR_SOAP_GEN_PAR_ERROR                    -48    // Soap parameter generate error              SOAP�������ɴ���
#define UDRM_ERROR_SOAP_GEN_REQUEST_ERROR                -49    // Soap request generate error                SOAP�������ɴ���
#define UDRM_ERROR_SOAP_PARSE_RESPONSE_ERROR             -50    // Soap response parse error                  SOAP��Ӧ��������
#define UDRM_ERROR_HTTP_REQUEST_ERROR                    -51    // HTTP request error                         HTTP�����쳣
#define UDRM_ERROR_GEN_PKEY_ERROR                        -52    // Private key generate error                 ˽Կ�����쳣
#define UDRM_ERROR_GEN_CERTREQ_ERROR                     -53    // Cert request generate error                ֤�����������쳣
#define UDRM_ERROR_DEVICE_NO_RIGHT                       -54    // Device no right                            �豸��Ȩ��
#define UDRM_ERROR_CERT_TYPE_INVALID                     -55    // Cert type invalid                          ֤��������Ч
#define UDRM_ERROR_BOSS_CONTENT_NOT_EXIST                -56    // BOSS content not exist                     BOSS���ݲ�����
#define UDRM_ERROR_BOSS_DEVICE_NOT_EXIST                 -57    // BOSS device not exist                      BOSS�豸������
#define UDRM_ERROR_BOSS_DEVICE_INVALID                   -58    // BOSS device invalid                        BOSS�豸��Ч
#define UDRM_ERROR_BOSS_DEVICE_NO_RIGHT                  -59    // BOSS device no right                       BOSS�豸��Ȩ��
#define UDRM_ERROR_BOSS_CREDIT_NOT_ENOUGH                -60    // BOSS credit not enough                     BOSS�ʽ���
#define UDRM_ERROR_BOSS_CONTENT_NO_RIGHT                 -61    // BOSS content no right                      BOSS������Ȩ��
#define UDRM_ERROR_BOSS_OPERATE_ERROR                    -62    // BOSS operate error                         BOSS�����쳣
#define UDRM_ERROR_SEND_ERROR                            -63    // Send data error                            ���ݷ����쳣
#define UDRM_ERROR_RECV_ERROR                            -64    // Receive data error                         ���ݽ����쳣
#define UDRM_ERROR_SSL_NEW_ERROR                         -65    // SSL new error                              SSL����ʧ��
#define UDRM_ERROR_SSL_USE_CERT_ERROR                    -66    // SSL use cert error                         SSL����֤��ʧ��
#define UDRM_ERROR_SSL_USE_PKEY_ERROR                    -67    // SSL use private key error                  SSL����˽Կʧ��
#define UDRM_ERROR_SSL_CONNECT_ERROR                     -68    // SSL connect error                          SSL����ʧ��
#define UDRM_ERROR_SSL_SERVER_CERT_INVALID               -69    // SSL server cert check error                SSL������֤����֤����
#define UDRM_ERROR_TIMESTAMP_ERROR                       -70    // Timestamp error                            ʱ�������
#define UDRM_ERROR_SIGNATURE_CERT_NO_TRUST               -71    // Certificate signature not trusted          ֤��ǩ��������
#define UDRM_ERROR_SIGNATURE_PROCESS_ERROR               -72    // Security processing failed                 ǩ���������
#define UDRM_ERROR_CERTNAME_EXIST                        -73    // Cert name already exist                    ֤�����Ѵ���
#define UDRM_ERROR_MY_PKEY_ERROR                         -74    // Myself private key error                   �ҵ�˽Կ�쳣
#define UDRM_ERROR_LOAD_CERT_ERROR                       -75    // Load cert or cert link error               ֤������쳣
#define UDRM_ERROR_SSL_CERT_ERROR                        -76    // SSL connect cert error                     SSL����֤�����
#define UDRM_ERROR_SSL_PKEY_ERROR                        -77    // SSL connect pkey error                     SSL����˽Կ����
#define UDRM_ERROR_DEVICE_ID_ERROR                       -78    // Device id error                            �豸ID����
#define UDRM_ERROR_DEVICE_TYPE_ERROR                     -79    // Device type error                          �豸���ʹ���
#define UDRM_ERROR_DEVICE_NO_INIT                        -80    // Device no initial                          �豸δ��ʼ��
#define UDRM_ERROR_DEVICE_BIND_NOT_MATCH                 -81    // Device bind not match                      �豸�󶨲�ƥ��
#define UDRM_ERROR_BOSS_USERID_ERROR                     -82    // BOSS user id error                         BOSS�û�������
#define UDRM_ERROR_BOSS_PASSWORD_ERROR                   -83    // BOSS user password error                   BOSS�������
#define UDRM_ERROR_DEVICE_USER_ERROR                     -84    // Device bind user error                     �豸���û�����
#define UDRM_ERROR_DEVICE_MAC_ERROR                      -85    // Device bind mac error                      �豸��MAC����
#define UDRM_ERROR_BOSS_USER_NO_RIGHT                    -86    // BOSS user no right                         BOSS�û���Ȩ��
#define UDRM_ERROR_BOSS_BIND_NUM_OVER                    -87    // BOSS bind number over                      BOSS�󶨳���
#define UDRM_ERROR_BOSS_NO_RIGHT                         -88    // BOSS no right                              BOSS��Ȩ��
#define UDRM_ERROR_BOSS_NOT_SUPPORT                      -89    // BOSS not support                           BOSS���ܲ�֧��
#define UDRM_ERROR_BOSS_BIND_ERROR                       -90    // BOSS bind error                            BOSS�󶨴���
#define UDRM_ERROR_BOSS_USERID_ALREADY_EXIST             -91    // BOSS user id already exist                 BOSS�û�ID�Ѵ���
#define UDRM_ERROR_BOSS_USER_DEVICE_NOT_MATCH            -92    // BOSS user device not match                 BOSS�û��豸��ƥ��
#define UDRM_ERROR_BOSS_VALIDATION_ERROR                 -93    // BOSS validation error                      BOSS��֤�����
#define UDRM_ERROR_BOSS_DEVICE_ID_ALREADY_EXIST          -94    // BOSS device id already exist               BOSS�豸ID�Ѵ���
#define UDRM_ERROR_BOSS_DEVICE_TYPE_ERROR                -95    // BOSS device type error                     BOSS�豸���ʹ���
#define UDRM_ERROR_BOSS_DEVICE_ALREADY_REGISTER          -96    // BOSS device already register               BOSS�豸��ע��
#define UDRM_ERROR_BOSS_DEVICE_ALREADY_BIND              -97    // BOSS device already bind                   BOSS�豸�Ѱ�
#define UDRM_ERROR_BOSS_CONTENT_ID_ALREADY_EXIST         -98    // BOSS content id already exist              BOSS����ID�Ѵ���
#define UDRM_ERROR_BOSS_DB_OPERATE_ERROR                 -99    // BOSS db operate error                      BOSS���ݿ�����쳣
#define UDRM_ERROR_BOSS_SERVICE_CONNECT_ERROR            -100   // BOSS service connect error                 BOSS���������쳣
#define UDRM_ERROR_BOSS_SERVICE_OPERATE_ERROR            -101   // BOSS service operate error                 BOSS��������쳣
#define UDRM_ERROR_BOSS_LICENSE_ID_ERROR                 -102   // BOSS license id error                      BOSS���֤ID����
#define UDRM_ERROR_BOSS_USER_NOT_MATCH                   -103   // BOSS user not match                        BOSS�û���ƥ��
#define UDRM_ERROR_LICENSE_PLAYCOUNT_OVER                -104   // License play count over                    ��ɴ�������
#define UDRM_ERROR_LICENSE_PLAYTIME_OVER                 -105   // License play time over                     ���ʱ������
#define UDRM_ERROR_LICENSE_PLAYPERIOD_OVER               -106   // License play period over                   ���ʱ��γ���
#define UDRM_ERROR_LICENSE_HMAC_ERROR                    -107   // License HMAC error                         ���֤HMAC����
#define UDRM_ERROR_LICENSE_TIME_INVALID                  -108   // License time invalid                       ���֤ʱ����Ч
#define UDRM_ERROR_LICENSE_CHECK_TIME_OVER               -109   // License check time over                    ���֤���ʱ�䳬��
#define UDRM_ERROR_LICENSE_CONTENT_NOT_MATCH             -110   // License check content not match            ������ݲ�ƥ��
#define UDRM_ERROR_URL_ERROR                             -111   // URL error                                  URL����
#define UDRM_ERROR_PROTOCOL_ERROR                        -112   // Protocol error                             Э�����
#define UDRM_ERROR_HOSTNAME_ERROR                        -113   // Host name error                            ����������
#define UDRM_ERROR_BAD_REQUEST                           -114   // 400 Bad Request                            400�������
#define UDRM_ERROR_SERVICE_TEMP_UNAVAILABLE              -115   // 503 Service Temporarily Unavailable        503������ʱ��Ч
#define UDRM_ERROR_USERID_EMPTY                          -116   // User id is empty                           �û�IDΪ��
#define UDRM_ERROR_PASSWORD_EMPTY                        -117   // Password is empty                          ����Ϊ��
#define UDRM_ERROR_AGENT_SERVER_TIME_ERROR               -118   // Agent server time error                    DRMʱ�����
#define UDRM_ERROR_GROUP_ID_ERROR                        -119   // Group id error                             ��ID����
#define UDRM_ERROR_GROUP_ID_NOT_EXIST                    -120   // Group id not exist                         �鲻����
#define UDRM_ERROR_GROUP_ID_ALREADY_EXIST                -121   // Group id already exist                     ��ID�Ѵ���
#define UDRM_ERROR_GROUP_NO_RIGHT                        -122   // Group no right                             ����Ȩ��
#define UDRM_ERROR_GROUP_NUM_OVER                        -123   // Group number over                          ���û�����
#define UDRM_ERROR_GROUP_CERT_ERROR                      -124   // Group cert error                           ��֤�����
#define UDRM_ERROR_GROUP_PKEY_ERROR                      -125   // Group private key error                    ��˽Կ����
#define UDRM_ERROR_DEVICE_FILE_ERROR                     -126   // Device file error                          �豸�ļ�����
#define UDRM_ERROR_DEVICE_FILE_OPEN_ERROR                -127   // Device file open error                     �豸�ļ����쳣
#define UDRM_ERROR_DEVICE_FILE_READ_ERROR                -128   // Device file read error                     �豸�ļ���ȡ�쳣
#define UDRM_ERROR_DEVICE_FILE_WRITE_ERROR               -129   // Device file write error                    �豸�ļ�д���쳣
#define UDRM_ERROR_DEVICE_FILE_CLOSE_ERROR               -130   // Device file close error                    �豸�ļ��ر��쳣
#define UDRM_ERROR_DEVICE_DATA_TYPE_ERROR                -131   // Device data type error                     �豸�������ʹ���
#define UDRM_ERROR_RULE_NOT_EXIST                        -132   // Rule is not exist                          ʹ�ù��򲻴���
#define UDRM_ERROR_RULE_ALREADY_EXIST                    -133   // Rule already exist                         ʹ�ù����Ѵ���
#define UDRM_ERROR_RULE_INVALID                          -134   // Rule is invalid                            ʹ�ù�����Ч
#define UDRM_ERROR_RULE_UPDATE                           -135   // Rule need update                           ʹ�ù��������
#define UDRM_ERROR_RULE_EXPIRATION                       -136   // Rule is expiration                         ʹ�ù����ѹ���
#define UDRM_ERROR_GZIP_ERROR                            -137   // Gzip error                                 GZIP����
#define UDRM_ERROR_GZIP_NOT_SUPPORT                      -138   // Gzip not support                           GZIP��֧��
#define UDRM_ERROR_ENV_ERROR                             -139   // Env value error                            ������������
#define UDRM_ERROR_ENV_EMPTY                             -140   // Env is empty                               ��������Ϊ��
#define UDRM_ERROR_NOT_MATCH                             -141   // Information not match                      ��Ϣ��ƥ��
#define UDRM_ERROR_PARSE_XML_ERROR                       -142   // Parse xml error                            XML�����쳣
#define UDRM_ERROR_SYNC_EVENT_ERROR                      -143   // Sync event error                           �¼�ͬ���쳣
#define UDRM_ERROR_BOSS_UNBIND_ERROR                     -144   // BOSS unbind error                          BOSS������
#define UDRM_ERROR_BOSS_NOTBIND_ERROR                    -145   // BOSS not bind                              BOSSδ��
#define UDRM_ERROR_BOSS_URL_ERROR                        -146   // BOSS URL error                             BOSS URL����
#define UDRM_ERROR_TRANSFER_CERT_ERROR                   -147   // Transfer cert error                        ����֤�����
#define UDRM_ERROR_REQUEST_CERT_ERROR                    -148   // Request cert error                         ����֤�����
#define UDRM_ERROR_CONTENT_NO_RIGHT                      -149   // Content no right                           ������Ȩ��
#define UDRM_ERROR_BIND_ERROR                            -150   // Bind error                                 ���쳣
#define UDRM_ERROR_UNBIND_ERROR                          -151   // Unbind error                               ����쳣
#define UDRM_ERROR_NOT_BIND_ERROR                        -152   // Not bind error                             δ��
#define UDRM_ERROR_VERSION_ERROR                         -153   // Version error                              �汾����
#define UDRM_ERROR_VERSION_NOT_SUPPORT                   -154   // Version not support                        �汾��֧��
#define UDRM_ERROR_VERSION_NEED_UPDATE                   -155   // Version need update                        �汾�����
#define UDRM_ERROR_TRANSFER_DCERT_NOT_MATCH              -156   // Transfer device cert not match             �����豸֤�鲻ƥ��
#define UDRM_ERROR_NEED_BIND                             -157   // Need bind                                  ��Ҫ��
#define UDRM_ERROR_UG_NOT_EXIST                          -158   // UG not exist                               ���ز�����
#define UDRM_ERROR_UG_ALREADY_EXIST                      -159   // UG already exist                           �����Ѵ���
#define UDRM_ERROR_UG_NO_RIGHT                           -160   // UG no right                                ������Ȩ��
#define UDRM_ERROR_UG_BIND_ERROR                         -161   // UG bind error                              ���ذ󶨴���
#define UDRM_ERROR_UG_UNBIND_ERROR                       -162   // UG unbind error                            ���ؽ�����
#define UDRM_ERROR_UG_ALREADY_REGISTER                   -163   // UG already register                        ������ע��
#define UDRM_ERROR_UG_ALREADY_BIND                       -164   // UG already bind                            �����Ѱ�
#define UDRM_ERROR_UG_NOT_BIND                           -165   // UG not bind                                ����δ��
#define UDRM_ERROR_UG_BIND_NUM_OVER                      -166   // UG bind number over                        ���ذ�������
#define UDRM_ERROR_UG_TYPE_ERROR                         -167   // UG type error                              �������ʹ���
#define UDRM_ERROR_BOSS_UG_NOT_EXIST                     -168   // BOSS UG not exist                          BOSS���ز�����
#define UDRM_ERROR_BOSS_UG_ALREADY_EXIST                 -169   // BOSS UG already exist                      BOSS�����Ѵ���
#define UDRM_ERROR_BOSS_UG_NO_RIGHT                      -170   // BOSS UG no right                           BOSS������Ȩ��
#define UDRM_ERROR_BOSS_UG_BIND_ERROR                    -171   // BOSS UG bind error                         BOSS���ذ󶨴���
#define UDRM_ERROR_BOSS_UG_UNBIND_ERROR                  -172   // BOSS UG unbind error                       BOSS���ؽ�����
#define UDRM_ERROR_BOSS_UG_ALREADY_REGISTER              -173   // BOSS UG already register                   BOSS������ע��
#define UDRM_ERROR_BOSS_UG_ALREADY_BIND                  -174   // BOSS UG already bind                       BOSS�����Ѱ�
#define UDRM_ERROR_BOSS_UG_NOT_BIND                      -175   // BOSS UG not bind                           BOSS����δ��
#define UDRM_ERROR_BOSS_UG_BIND_NUM_OVER                 -176   // BOSS UG bind number over                   BOSS���ذ�������
#define UDRM_ERROR_BOSS_UG_TYPE_ERROR                    -177   // BOSS UG type error                         BOSS�������ʹ���
#define UDRM_ERROR_PROGRAM_NOT_EXIST                     -178   // Program not exist                          �ײͲ�����
#define UDRM_ERROR_PROGRAM_ALREADY_EXIST                 -179   // Program already exist                      �ײ��Ѵ���
#define UDRM_ERROR_PROGRAM_ERROR                         -180   // Program error                              �ײ��쳣
#define UDRM_ERROR_PROGRAM_NO_RIGHT                      -181   // Program no right                           �ײ���Ȩ��
#define UDRM_ERROR_BOSS_PROGRAM_NOT_EXIST                -182   // BOSS Program not exist                     BOSS�ײͲ�����
#define UDRM_ERROR_BOSS_PROGRAM_ALREADY_EXIST            -183   // BOSS Program already exist                 BOSS�ײ��Ѵ���
#define UDRM_ERROR_BOSS_PROGRAM_ERROR                    -184   // BOSS Program error                         BOSS�ײ��쳣
#define UDRM_ERROR_BOSS_PROGRAM_NO_RIGHT                 -185   // BOSS Program no right                      BOSS�ײ���Ȩ��
#define UDRM_ERROR_SUB_NOT_EXIST                         -186   // Subscription not exist                     ����������
#define UDRM_ERROR_SUB_ALREADY_EXIST                     -187   // Subscription already exit                  �����Ѿ�����
#define UDRM_ERROR_SUB_ERROR                             -188   // Subscription error                         �����쳣
#define UDRM_ERROR_SUB_NO_RIGHT                          -189   // Subscription no right                      ������Ȩ��
#define UDRM_ERROR_BOSS_SUB_NOT_EXIST                    -190   // BOSS Subscription not exist                BOSS����������
#define UDRM_ERROR_BOSS_SUB_ALREADY_EXIST                -191   // BOSS Subscription already exit             BOSS�����Ѿ�����
#define UDRM_ERROR_BOSS_SUB_ERROR                        -192   // BOSS Subscription error                    BOSS�����쳣
#define UDRM_ERROR_BOSS_SUB_NO_RIGHT                     -193   // BOSS Subscription no right                 BOSS������Ȩ��
#define UDRM_ERROR_SSL_SESSION_ERROR                     -194   // SSL session error                          SSL Session �쳣

#define UDRM_ERROR_NONE									 -195 /*use to count the total error message*/


#define UDRM_DEVICELIST_ACCOUNTNAME_LEN                  256
#define UDRM_DEVICELIST_ACCOUNTPASSWORD_LEN              256
#define UDRM_DEVICELIST_DRMID_LEN                        64
#define UDRM_DEVICELIST_MACADDR_LEN                      256
#define UDRM_DEVICELIST_DEVICENAME_LEN                   256


#define UDRM_CERT_TYPE_ROOT                        0
#define UDRM_CERT_TYPE_DEVICE                      1
#define UDRM_CERT_TYPE_ACS                         2
#define UDRM_CERT_TYPE_KMS                         3
#define UDRM_CERT_TYPE_CAS                         4
#define UDRM_CERT_TYPE_OCSP                        5
#define UDRM_CERT_TYPE_BROWSER                     6
#define UDRM_CERT_TYPE_SERVER                      7
#define UDRM_CERT_TYPE_TEST                        8

#define UDRMCA_REQ_COUNTRY_NAME_LEN                16
#define UDRMCA_REQ_STATE_OR_PROVINCE_NAME_LEN      32
#define UDRMCA_REQ_LOCALITY_NAME_LEN               64
#define UDRMCA_REQ_ORGANIZATION_NAME_LEN           64
#define UDRMCA_REQ_ORGANIZATION_UNIT_NAME_LEN      64
#define UDRMCA_REQ_COMMON_NAME_LEN                 64
#define UDRMCA_REQ_EMAIL_ADDRESS_LEN               24
#define UDRMCA_REQ_EMAIL_PROTECT_LEN               24
#define UDRMCA_REQ_TITLE_LEN                       12
#define UDRMCA_REQ_DESCRIPTION_LEN                 12
#define UDRMCA_REQ_GIVEN_NAME_LEN                  12
#define UDRMCA_REQ_INITIALS_LEN                    64
#define UDRMCA_REQ_NAME_LEN                        12
#define UDRMCA_REQ_SURNAME_LEN                     12
#define UDRMCA_REQ_DN_QUALIFIRE_LEN                12
#define UDRMCA_REQ_UNSTRUCTURED_NAME_LEN           12
#define UDRMCA_REQ_CHALLENGE_PASSWORD_LEN          12
#define UDRMCA_REQ_UNSTRUCTURE_ADDRESS_LEN         12
#define UDRMCA_REQ_UDRM_ID_LEN                     32

#define UDRM_CA_EXT_BASIC_CONSTRAINTS_LEN          512
#define UDRM_CA_EXT_NS_COMMENT_LEN                 512
#define UDRM_CA_EXT_SUBJECT_KEY_IDENTIFIRE_LEN     512
#define UDRM_CA_EXT_AUTHORITY_KEY_IDENTIFIRE_LEN   512
#define UDRM_CA_EXT_KEY_USAGE_LEN                  512
#define UDRM_CA_EXT_EKEY_USAGE_LEN                 512
#define UDRM_CA_EXT_CRL_DISTRIBUTION_POINTS_LEN    512
#define UDRM_CA_EXT_AUTHORITY_INFO_ACCESS_LEN      512
#define UDRM_CA_EXT_POLICY_CONSTRAINT_LEN          512


#define UDRMCA_CERT_SUBJECT_LEN                     256
#define UDRMCA_CERT_COUNTRY_NAME_LEN                16
#define UDRMCA_CERT_STATE_OR_PROVINCE_NAME_LEN      32
#define UDRMCA_CERT_LOCALITY_NAME_LEN               64
#define UDRMCA_CERT_ORGANIZATION_NAME_LEN           64
#define UDRMCA_CERT_ORGANIZATION_UNIT_NAME_LEN      64
#define UDRMCA_CERT_COMMON_NAME_LEN                 64
#define UDRMCA_CERT_UDRM_ID_LEN                     32
#define UDRMCA_CERT_NOT_BEFORE_LEN                  64
#define UDRMCA_CERT_NOT_AFTER_LEN                   64
#define UDRMCA_CERT_ISSUER_SUBJECT_LEN              256
#define UDRMCA_CERT_SIGN_ALGORITHM_LEN              64


#define UDRM_CA_CERT_MAX_LEN                       8192
#define UDRM_CA_REQ_MAX_LEN                        4096
#define UDRM_CA_KEY_MAX_LEN                        4096
#define UDRM_LICENSE_MAX_LEN                       8192

#define UDRM_CRYPTO_AES_KEY_LEN                    16

#define UDRM_DEVICE_TYPE_NONE                      -1
#define UDRM_DEVICE_TYPE_HARDWARE                  0
#define UDRM_DEVICE_TYPE_SOFT                      1

#define UDRM_MAX_MSG_LEGNTH 1024

typedef enum _Decrypt_Status
{
	UDRM_DECRYPT_NONE =0,
	UDRM_DECRYPT_ACCESSING_LICENSE,
	UDRM_DECRYPT_DECRYPTING,
	UDRM_DECRYPT_ERROR,
	UDRM_DECRYPT_STOPPED,
} decrypt_status;

typedef struct ContentRule_st {
	UTI_INT playFlag;
	UTI_INT playMethod;
	UTI_INT playCount;
	UTI_INT playTime;
	UTI_LONG playStartTime;
	UTI_LONG playEndTime;
	UTI_INT contentRecFlag;
	UTI_INT contentStoreFlag;
	UTI_INT rightStoreFlag;
	UTI_INT contentTransferFlag;
	UTI_INT rightTransferFlag;
	UTI_INT parentControlFlag;
}ContentRule, *PContentRule;

typedef struct UDRMDevice_st
{
	UTI_CHAR pchAccountName[UDRM_DEVICELIST_ACCOUNTNAME_LEN];
	UTI_CHAR pchAccountPassword[UDRM_DEVICELIST_ACCOUNTPASSWORD_LEN];
	UTI_CHAR pchDRMID [UDRM_DEVICELIST_DRMID_LEN];
	UTI_CHAR pchMACAddr[UDRM_DEVICELIST_MACADDR_LEN];
	UTI_CHAR pchDeviceName[UDRM_DEVICELIST_DEVICENAME_LEN];
	UTI_INT nStatus;
	struct UDRMDevice_st * next;
}UDRMDevice, *PUDRMDevice;

typedef struct UDRMDeviceList_st {
	UTI_INT count;
	UDRMDevice * first;
}UDRMDeviceList, *PUDRMDeviceList;

typedef enum _UDRM_Stage
{
	UDRM_STAGE_NONE = 0,
	UDRM_STAGE_DRM_AGENT_CERT_NOT_FOUND,//CERT INIT
	UDRM_STAGE_DRM_AGENT_CERT_INIT_START,
	UDRM_STAGE_DRM_AGENT_CERT_INITINIZING,
	UDRM_STAGE_DRM_AGENT_CERT_INIT_SUCCESS,
	UDRM_STAGE_DRM_AGENT_CERT_INIT_FAILED,
	UDRM_STAGE_DRM_AGENT_CERT_INVALID_FOUND,//CERT UPDATE
	UDRM_STAGE_DRM_AGENT_CERT_UPDATE_START,
	UDRM_STAGE_DRM_AGENT_CERT_UPDATING,
	UDRM_STAGE_DRM_AGENT_CERT_UPDATE_SUCCESS,
	UDRM_STAGE_DRM_AGENT_CERT_UPDATE_FAILED,
	UDRM_STAGE_DRM_AGENT_INIT_START,//AGENT INIT
	UDRM_STAGE_DRM_AGENT_INIT_SUCCESS,
	UDRM_STAGE_DRM_AGENT_INIT_FAILED,
	UDRM_STAGE_DRM_CONTENT_FOUND,
	UDRM_STAGE_START_ACCESS_LICENSE,//LICENSE ACCESS
	UDRM_STAGE_DRM_SESSION_CREATED,
	UDRM_STAGE_DRM_SESSION_CREATE_FAILED,
	UDRM_STAGE_ACCESSING_LICENSE,
	UDRM_STAGE_ACCESS_LICENSE_FAILED_RETRY,
	UDRM_STAGE_ACCESS_LICENSE_SUCCESS,
	UDRM_STAGE_ACCESS_LICENSE_FAILED,
	UDRM_STAGE_DRM_SESSION_CLOSED,
	UDRM_STAGE_DRM_AGENT_DESTORY,
} UDRM_Stage;


typedef UTI_SINT32 (*UDRMMsgCallback)(IN UTI_UINT8 *pu8Msg, IN UTI_UINT32 u32Len, IN UTI_SINT32 s32UDRMError, IN decrypt_status status, IN UTI_VOID* pCallbackData);
UTI_SINT32 UDRMPlaySetUDRMMsgFunc(IN UTI_VOID* u32PlayHandle,  IN UDRMMsgCallback pfUDRMMsgCallback, IN UTI_VOID* pCallbackData);
//UTI_SINT32	UDRMMsg(IN UTI_UINT8 *pu8Msg, IN UTI_UINT32 u32Len, IN UTI_SINT32 s32UDRMError, IN decrypt_status status, IN UTI_VOID* pCallbackData);


UTI_RESULT UDRMAgentInit();

UTI_RESULT UDRMAgentInitLog(UTI_INT logtype, UTI_INT loglevel, UTI_CHAR * logfile);
UTI_INT UDRMAgentInitDeviceLocalPath(UTI_CHAR * localpath);

UTI_RESULT UDRMAgentSetEnv(IN UTI_CHAR * pchAccountName, IN UTI_CHAR *pchAccountPassword, IN UTI_CHAR *pchMACAddr, IN UTI_CHAR *pchDeviceName,IN UTI_CHAR *pchDRMURL);

UTI_RESULT UDRMAgentCheckBindStatus();

UTI_RESULT UDRMAgentBindDevice(IN UTI_CHAR *pchAccountName, IN UTI_CHAR *pchAccountPassword, IN UTI_CHAR *pchMACAddr, IN UTI_CHAR *pchDeviceName);

UTI_VOID* UDRMAgentDecryptStart(IN UTI_UINT16 u16ProgramNumber,IN UTI_CHAR *pchAuthorizationURL);



UTI_SINT32 UDRMAgentDecryptTS(IN UTI_VOID* u32PlayHandle, IN UTI_UINT8 *pu8InTsPackets,
								 IN UTI_UINT32 u32InPacketsLength, OUT UTI_UINT8 *pu8OutTsPackets, IN UTI_UINT32 u32OutPacketsLength);

UTI_SINT32 UDRMAgentDecryptStop(IN UTI_VOID* u32PlayHandle);

UTI_VOID UDRMAgentDestroy();

UTI_SINT32 UDRMSetMp4PsshData(IN UTI_VOID* u32PlayHandle, IN UTI_UINT8 *pu8InTsPackets,IN UTI_UINT32 u32InPacketsLength);
UTI_SINT32 UDRMPlayLoadMp4_data(IN UTI_VOID* u32PlayHandle,IN char* pc_iv_value,IN char* encdata, IN UTI_UINT32 buflong, OUT char *outdecdata);

UTI_RESULT UDRMAgentGetCertCommonName(OUT UTI_CHAR *pchCommonName, IN UTI_UINT32 u32CommanNameLength);
UTI_RESULT UDRMAgentGetCertValidTime(OUT UTI_CHAR *pchValidFrom, IN UTI_UINT32 u32ValidFromLength, OUT UTI_CHAR *pchValidTo, IN UTI_UINT32 u32ValidToLength);

UTI_INT UDRMAgentGetDRMID(UTI_CHAR * DRMID, UTI_UINT DRMIDBuflen);

UTI_RESULT UDRMAgentGetErrorType(OUT UTI_CHAR *pchErrorMsg,  IN UTI_UINT32 u32ErrorMsgLength);
UTI_RESULT UDRMAgentGetErrorNumber(OUT UTI_CHAR *pchErrorMsgDebug,  IN UTI_UINT32 u32ErrorMsgDebugLength);

UTI_RESULT UDRMAgentGetDRMVersion(OUT UTI_CHAR *pchDRMVersion,IN UTI_UINT32 u32DRMVersionLength);



UTI_SINT32 UDRMPlayGetContentID(UTI_VOID* pu32PlayHandle, UTI_CHAR* pu8ContentID, UTI_INT nContentIDLength);

#define UDRM_LICENSE_ID_LEN                        64
#define UDRM_LICENSE_CID_LEN                       64
#define UDRM_LICENSE_USN_LEN                       64


UTI_INT		UDRMAgentInitializeDevice();
UTI_INT		UDRMAgentCheckDeviceStatus();



#endif  /* _UTI_TYPE_H_ */
