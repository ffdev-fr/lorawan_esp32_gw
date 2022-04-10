/*****************************************************************************************//**
 * @file     LoraTransceiverItf.c
 *
 * @author   F.Fargon 
 *
 * @version  V1.0
 *
 * @date     16/02/2018
 *
 * @brief    A LoraTransceiver manages the hardware used for radio transmission with LoRa protocol.  
 *
 * @details  This class is the interface exposed by objects implementing the low level layer of 
 *           LoRa packet transmission (i.e. the drivers of Semtech SX12xx chips).\n
 *           In current implementation, this interface is provided by the CSX1276 object.
 *
 * @note     This class IS THREAD SAFE.
*********************************************************************************************/

/*********************************************************************************************
  Espressif framework includes
*********************************************************************************************/

#include <Common.h>

/*********************************************************************************************
  Includes for object implementation
*********************************************************************************************/

#define LORATRANSCEIVERITF_IMPL

#include "LoraTransceiverItf.h"


/*********************************************************************************************
  Interface object construction (i.e. 'ILoraTransceiver' class instance)
 
  Protected methods used by object implementing the interface
*********************************************************************************************/

/*****************************************************************************************//**
 * @fn         ILoraTransceiver ILoraTransceiver_New(void *pOwnerObject, 
 *                                                   CLoraTransceiverItfImpl pOwnerItfImpl)
 * 
 * @brief      Object construction. 
 * 
 * @details    Construction allowed only for objects implementing the 'ILoraTransceiver' 
 *             interface ('CSX1276', ...) (protected constructor)
 * 
 * @param      pOwnerObject
 *             Object implementing the interface (i.e. which provides interface to client objects).\n
 * 
 * @param      pOwnerItfImpl
 *             Array of pointers to ILoraTransceiver implementation functions in parent object.
 *  
 * @return     Pointer to the new 'ILoraTransceiver' instance.
*********************************************************************************************/
ILoraTransceiver ILoraTransceiver_New(void *pOwnerObject, CLoraTransceiverItfImpl pOwnerItfImpl)
{
  ILoraTransceiver this;

  if ((this = (void *) pvPortMalloc(sizeof(ILoraTransceiverOb))) != NULL)
  {
    // Set member variable values
    this->m_pOwnerObject = pOwnerObject;
    this->m_pOwnerItfImpl = pOwnerItfImpl;
  }
  return this;
}

/*****************************************************************************************//**
 * @fn         void ILoraTransceiver_Delete(ILoraTransceiver this)
 * 
 * @brief      Object destruction.
 * 
 * @details    Destroys the interface object.\n
 *             This method is allowed only for objects implementing the 'ILoraTransceiver' 
 *             interface ('CSX1276', ...) (protected destructor).\n
 *             It is called by owner object when interface reference count reaches 0. 
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     None.
*********************************************************************************************/
void ILoraTransceiver_Delete(ILoraTransceiver this)
{
  vPortFree(this);
}

/*********************************************************************************************
  Public methods of 'ILoraTransceiver' interface
 
  These methods are available to any object which has a reference to 'ILoraTransceiver'
  interface
*********************************************************************************************/


/*****************************************************************************************//**
 * @fn         uint32_t ILoraTransceiver_AddRef(ILoraTransceiver this)
 * 
 * @brief      Increments the interface's reference count.
 * 
 * @details    This function invokes the implementation of 'AddRef' method on owner object.\n
 *             The 'AddRef' method is invoked by client objects to make sure that interface will 
 *             be valid until they need it (i.e. client object invokes 'ReleaseItf' when it do
 *             not need the interface anymore).
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     The value of reference count once incremented.
*********************************************************************************************/
uint32_t ILoraTransceiver_AddRef(ILoraTransceiver this)
{
  this->m_pOwnerItfImpl->m_pAddRef(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         uint32_t ILoraTransceiver_ReleaseItf(ILoraTransceiver this)
 * 
 * @brief      Increments the interface's reference count.
 * 
 * @details    This function invokes the implementation of 'ReleasItf' method on owner object.\n
 *             The 'ReleaseItf' method is invoked by client objects when it do not need the 
 *             interface anymore.\n
 *             Typically, the owner object destroys the interface and itself when no more object
 *             is requiring it.
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     The value of reference count once decremented.
*********************************************************************************************/
uint32_t ILoraTransceiver_ReleaseItf(ILoraTransceiver this)
{
  this->m_pOwnerItfImpl->m_pReleaseItf(this->m_pOwnerObject);
  return true;
}

/*****************************************************************************************//**
 * @fn         bool ILoraTransceiver_Initialize(ILoraTransceiver this)
 * 
 * @brief      Initializes the Lora Transceiver.
 * 
 * @details    This function invokes the implementation of 'Initialize' method on owner object.\n
 *             The 'Initialize' method is invoked by client object during startup to reset the
 *             LoRa radio device and to configure it on its default state (typically standby 
 *             without radio activity).
 * 
 * @param      this
 *             The object pointer.
 *  
 * @return     The function returns 'true' if the LoRa Transceiver is ready or 'false' if an
 *             error has occured.
 * 
 * @note       Typically the Lora Transceiver is managed by a single owner objet. Only this owner
 *             calls administrative methods like 'Initialize'.
*********************************************************************************************/
bool ILoraTransceiver_Initialize(ILoraTransceiver this)
{
  return this->m_pOwnerItfImpl->m_pInitialize(this->m_pOwnerObject);
}



