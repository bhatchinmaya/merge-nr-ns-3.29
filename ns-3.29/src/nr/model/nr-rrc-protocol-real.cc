/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2019 National Instruments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Chinmaya Bhat <chinmaya.krishnamurthy.bhat@ni.com>
 *         
 */

#include <ns3/fatal-error.h>
#include <ns3/log.h>
#include <ns3/nstime.h>
#include <ns3/node-list.h>
#include <ns3/node.h>
#include <ns3/simulator.h>

#include "nr-rrc-protocol-real.h"
#include <ns3/lte-ue-rrc.h>
#include <ns3/lte-enb-rrc.h>
//#include <ns3/lte-enb-net-device.h>
//#include <ns3/lte-ue-net-device.h>
//#include <ns3/mmwave-ue-net-device.h>
#include <ns3/mmwave-enb-net-device.h>

namespace ns3 {



NS_LOG_COMPONENT_DEFINE ("NrRrcProtocolReal");

/// RRC real message delay
const Time RRC_REAL_MSG_DELAY = MilliSeconds (0);  // is it compatible with NI API?

NS_OBJECT_ENSURE_REGISTERED (NrUeRrcProtocolReal);

NrUeRrcProtocolReal::NrUeRrcProtocolReal ()
  :  m_ueRrcSapProvider (0),
    m_enbRrcSapProvider (0)
{
	NS_LOG_FUNCTION (this);
  m_ueRrcSapUser = new MemberLteUeRrcSapUser<NrUeRrcProtocolReal> (this);
  m_completeSetupParameters.srb0SapUser = new LteRlcSpecificLteRlcSapUser<NrUeRrcProtocolReal> (this);
  m_completeSetupParameters.srb1SapUser = new LtePdcpSpecificLtePdcpSapUser<NrUeRrcProtocolReal> (this);
}

NrUeRrcProtocolReal::~NrUeRrcProtocolReal ()
{
	NS_LOG_FUNCTION (this);
}

void
NrUeRrcProtocolReal::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_ueRrcSapUser;
  delete m_completeSetupParameters.srb0SapUser;
  delete m_completeSetupParameters.srb1SapUser;
  m_rrc = 0;
}

TypeId
NrUeRrcProtocolReal::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NrUeRrcProtocolReal")
    .SetParent<Object> ()
    //.SetGroupName("nr")
    .AddConstructor<NrUeRrcProtocolReal> ()
  ;
  return tid;
}

void
NrUeRrcProtocolReal::SetLteUeRrcSapProvider (LteUeRrcSapProvider* p)
{
	NS_LOG_FUNCTION (this);
  m_ueRrcSapProvider = p;
}

LteUeRrcSapUser*
NrUeRrcProtocolReal::GetLteUeRrcSapUser ()
{
	NS_LOG_FUNCTION (this);
  return m_ueRrcSapUser;
}

void
NrUeRrcProtocolReal::SetUeRrc (Ptr<LteUeRrc> rrc)
{
	NS_LOG_FUNCTION (this);
  m_rrc = rrc;
}

void
NrUeRrcProtocolReal::DoSetup (LteUeRrcSapUser::SetupParameters params)
{
  NS_LOG_FUNCTION (this);

  m_setupParameters.srb0SapProvider = params.srb0SapProvider;
  m_setupParameters.srb1SapProvider = params.srb1SapProvider;
  m_ueRrcSapProvider->CompleteSetup (m_completeSetupParameters);
}

void NrUeRrcProtocolReal::DoSendRrcConnectionRequest(LteRrcSap::RrcConnectionRequest msg) {
 // initialize the RNTI and get the EnbLteRrcSapProvider for the
  // eNB we are currently attached to
	NS_LOG_FUNCTION (this);
  m_rnti = m_rrc->GetRnti ();
  SetEnbRrcSapProvider ();

  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionRequestHeader rrcConnectionRequestHeader;
  rrcConnectionRequestHeader.SetMessage (msg);

  packet->AddHeader (rrcConnectionRequestHeader);

  LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
  transmitPdcpPduParameters.pdcpPdu = packet;
  transmitPdcpPduParameters.rnti = m_rnti;
  transmitPdcpPduParameters.lcid = 0;

  m_setupParameters.srb0SapProvider->TransmitPdcpPdu (transmitPdcpPduParameters);

}

void
NrUeRrcProtocolReal::DoSendRrcConnectionSetupCompleted (LteRrcSap::RrcConnectionSetupCompleted msg)
{
	NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionSetupCompleteHeader rrcConnectionSetupCompleteHeader;
  rrcConnectionSetupCompleteHeader.SetMessage (msg);

  packet->AddHeader (rrcConnectionSetupCompleteHeader);

  LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
  transmitPdcpSduParameters.pdcpSdu = packet;
  transmitPdcpSduParameters.rnti = m_rnti;
  transmitPdcpSduParameters.lcid = 1;

  if (m_setupParameters.srb1SapProvider)
    {
      m_setupParameters.srb1SapProvider->TransmitPdcpSdu (transmitPdcpSduParameters);
    }
}

void
NrUeRrcProtocolReal::DoSendRrcConnectionReconfigurationCompleted (LteRrcSap::RrcConnectionReconfigurationCompleted msg)
{
  // re-initialize the RNTI and get the EnbLteRrcSapProvider for the
  // eNB we are currently attached to
	NS_LOG_FUNCTION (this);
  m_rnti = m_rrc->GetRnti ();
  SetEnbRrcSapProvider ();

  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReconfigurationCompleteHeader rrcConnectionReconfigurationCompleteHeader;
  rrcConnectionReconfigurationCompleteHeader.SetMessage (msg);

  packet->AddHeader (rrcConnectionReconfigurationCompleteHeader);

  LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
  transmitPdcpSduParameters.pdcpSdu = packet;
  transmitPdcpSduParameters.rnti = m_rnti;
  transmitPdcpSduParameters.lcid = 1;

  m_setupParameters.srb1SapProvider->TransmitPdcpSdu (transmitPdcpSduParameters);
}

void
NrUeRrcProtocolReal::DoSendMeasurementReport (LteRrcSap::MeasurementReport msg)
{
  // re-initialize the RNTI and get the EnbLteRrcSapProvider for the
  // eNB we are currently attached to
	NS_LOG_FUNCTION (this);
  m_rnti = m_rrc->GetRnti ();
  SetEnbRrcSapProvider ();

  Ptr<Packet> packet = Create<Packet> ();

  MeasurementReportHeader measurementReportHeader;
  measurementReportHeader.SetMessage (msg);

  packet->AddHeader (measurementReportHeader);

  LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
  transmitPdcpSduParameters.pdcpSdu = packet;
  transmitPdcpSduParameters.rnti = m_rnti;
  transmitPdcpSduParameters.lcid = 1;

  m_setupParameters.srb1SapProvider->TransmitPdcpSdu (transmitPdcpSduParameters);
}


void
NrUeRrcProtocolReal::DoSendRrcConnectionReestablishmentRequest (LteRrcSap::RrcConnectionReestablishmentRequest msg)
{
	NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReestablishmentRequestHeader rrcConnectionReestablishmentRequestHeader;
  rrcConnectionReestablishmentRequestHeader.SetMessage (msg);

  packet->AddHeader (rrcConnectionReestablishmentRequestHeader);

  LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
  transmitPdcpPduParameters.pdcpPdu = packet;
  transmitPdcpPduParameters.rnti = m_rnti;
  transmitPdcpPduParameters.lcid = 0;

  m_setupParameters.srb0SapProvider->TransmitPdcpPdu (transmitPdcpPduParameters);
}

void
NrUeRrcProtocolReal::DoSendRrcConnectionReestablishmentComplete (LteRrcSap::RrcConnectionReestablishmentComplete msg)
{
	NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReestablishmentCompleteHeader rrcConnectionReestablishmentCompleteHeader;
  rrcConnectionReestablishmentCompleteHeader.SetMessage (msg);

  packet->AddHeader (rrcConnectionReestablishmentCompleteHeader);

  LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
  transmitPdcpSduParameters.pdcpSdu = packet;
  transmitPdcpSduParameters.rnti = m_rnti;
  transmitPdcpSduParameters.lcid = 1;

  m_setupParameters.srb1SapProvider->TransmitPdcpSdu (transmitPdcpSduParameters);
}

void
NrUeRrcProtocolReal::SetEnbRrcSapProvider ()
{
	NS_LOG_FUNCTION (this);
uint16_t cellId = m_rrc->GetCellId ();

  // walk list of all nodes to get the peer eNB
  Ptr<MmWaveEnbNetDevice> enbDev;
  NodeList::Iterator listEnd = NodeList::End ();
  bool found = false;
  for (NodeList::Iterator i = NodeList::Begin (); 
       (i != listEnd) && (!found); 
       ++i)
    {
      Ptr<Node> node = *i;
      int nDevs = node->GetNDevices ();
      for (int j = 0; 
           (j < nDevs) && (!found);
           j++)
        {
          enbDev = node->GetDevice (j)->GetObject <MmWaveEnbNetDevice> ();
          if (enbDev == 0)
            {
              continue;
            }
          else
            {
            //  if (enbDev->HasCellId (cellId))
            if(enbDev->GetCellId () == cellId) //HasCellId not available
                {
                  found = true;
                  break;
                }
            }
        }
    }
  NS_ASSERT_MSG (found, " Unable to find eNB with CellId =" << cellId);
  m_enbRrcSapProvider = enbDev->GetRrc ()->GetLteEnbRrcSapProvider ();
  Ptr<NrEnbRrcProtocolReal> enbRrcProtocolReal = enbDev->GetRrc ()->GetObject<NrEnbRrcProtocolReal> ();
  enbRrcProtocolReal->SetUeRrcSapProvider (m_rnti, m_ueRrcSapProvider);
}

void
NrUeRrcProtocolReal::DoReceivePdcpPdu (Ptr<Packet> p)
{
	NS_LOG_FUNCTION (this);
  // Get type of message received
  RrcDlCcchMessage rrcDlCcchMessage;
  p->PeekHeader (rrcDlCcchMessage);

  // Declare possible headers to receive
  RrcConnectionReestablishmentHeader rrcConnectionReestablishmentHeader;
  RrcConnectionReestablishmentRejectHeader rrcConnectionReestablishmentRejectHeader;
  RrcConnectionSetupHeader rrcConnectionSetupHeader;
  RrcConnectionRejectHeader rrcConnectionRejectHeader;

  // Declare possible messages
  LteRrcSap::RrcConnectionReestablishment rrcConnectionReestablishmentMsg;
  LteRrcSap::RrcConnectionReestablishmentReject rrcConnectionReestablishmentRejectMsg;
  LteRrcSap::RrcConnectionSetup rrcConnectionSetupMsg;
  LteRrcSap::RrcConnectionReject rrcConnectionRejectMsg;

  // Deserialize packet and call member recv function with appropriate structure
  switch ( rrcDlCcchMessage.GetMessageType () )
    {
    case 0:
      // RrcConnectionReestablishment
      p->RemoveHeader (rrcConnectionReestablishmentHeader);
      rrcConnectionReestablishmentMsg = rrcConnectionReestablishmentHeader.GetMessage ();
      m_ueRrcSapProvider->RecvRrcConnectionReestablishment (rrcConnectionReestablishmentMsg);
      break;
    case 1:
      // RrcConnectionReestablishmentReject
      p->RemoveHeader (rrcConnectionReestablishmentRejectHeader);
      rrcConnectionReestablishmentRejectMsg = rrcConnectionReestablishmentRejectHeader.GetMessage ();
      // m_ueRrcSapProvider->RecvRrcConnectionReestablishmentReject (rrcConnectionReestablishmentRejectMsg);
      break;
    case 2:
      // RrcConnectionReject
      p->RemoveHeader (rrcConnectionRejectHeader);
      rrcConnectionRejectMsg = rrcConnectionRejectHeader.GetMessage ();
      m_ueRrcSapProvider->RecvRrcConnectionReject (rrcConnectionRejectMsg);
      break;
    case 3:
      // RrcConnectionSetup
      p->RemoveHeader (rrcConnectionSetupHeader);
      rrcConnectionSetupMsg = rrcConnectionSetupHeader.GetMessage ();
      m_ueRrcSapProvider->RecvRrcConnectionSetup (rrcConnectionSetupMsg);
      break;
    
    }
}


void
NrUeRrcProtocolReal::DoReceivePdcpSdu (LtePdcpSapUser::ReceivePdcpSduParameters params)
{
	NS_LOG_FUNCTION (this);
  // Get type of message received
  RrcDlDcchMessage rrcDlDcchMessage;
  params.pdcpSdu->PeekHeader (rrcDlDcchMessage);

  // Declare possible headers to receive
  RrcConnectionReconfigurationHeader rrcConnectionReconfigurationHeader;
  RrcConnectionReleaseHeader rrcConnectionReleaseHeader;

  // Declare possible messages to receive
  LteRrcSap::RrcConnectionReconfiguration rrcConnectionReconfigurationMsg;
  LteRrcSap::RrcConnectionRelease rrcConnectionReleaseMsg;

  // Deserialize packet and call member recv function with appropriate structure
  switch ( rrcDlDcchMessage.GetMessageType () )
    {
    case 4:
      params.pdcpSdu->RemoveHeader (rrcConnectionReconfigurationHeader);
      rrcConnectionReconfigurationMsg = rrcConnectionReconfigurationHeader.GetMessage ();
      m_ueRrcSapProvider->RecvRrcConnectionReconfiguration (rrcConnectionReconfigurationMsg);
      break;
    case 5:
      params.pdcpSdu->RemoveHeader (rrcConnectionReleaseHeader);
      rrcConnectionReleaseMsg = rrcConnectionReleaseHeader.GetMessage ();
      //m_ueRrcSapProvider->RecvRrcConnectionRelease (rrcConnectionReleaseMsg);
      break;
    }
}


NS_OBJECT_ENSURE_REGISTERED (NrEnbRrcProtocolReal);

NrEnbRrcProtocolReal::NrEnbRrcProtocolReal ()
  :  m_enbRrcSapProvider (0)
{
  NS_LOG_FUNCTION (this);
  m_enbRrcSapUser = new MemberLteEnbRrcSapUser<NrEnbRrcProtocolReal> (this);
}

NrEnbRrcProtocolReal::~NrEnbRrcProtocolReal ()
{
  NS_LOG_FUNCTION (this);
}

void
NrEnbRrcProtocolReal::DoDispose ()
{
  NS_LOG_FUNCTION (this);
  delete m_enbRrcSapUser;
  for (std::map<uint16_t, LteEnbRrcSapProvider::CompleteSetupUeParameters>::iterator
         it = m_completeSetupUeParametersMap.begin ();
       it != m_completeSetupUeParametersMap.end ();
       ++it)
    {
      delete it->second.srb0SapUser;
      delete it->second.srb1SapUser;
    }
  m_completeSetupUeParametersMap.clear ();
}

TypeId
NrEnbRrcProtocolReal::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NrEnbRrcProtocolReal")
    .SetParent<Object> ()
    //.SetGroupName("nr")
    .AddConstructor<NrEnbRrcProtocolReal> ()
  ;
  return tid;
}


void
NrEnbRrcProtocolReal::SetLteEnbRrcSapProvider (LteEnbRrcSapProvider* p)
{
	NS_LOG_FUNCTION (this);
  m_enbRrcSapProvider = p;
}

LteEnbRrcSapUser*
NrEnbRrcProtocolReal::GetLteEnbRrcSapUser ()
{
	NS_LOG_FUNCTION (this);
  return m_enbRrcSapUser;
}

void
NrEnbRrcProtocolReal::SetCellId (uint16_t cellId)
{
	NS_LOG_FUNCTION (this);
  m_cellId = cellId;
}

LteUeRrcSapProvider*
NrEnbRrcProtocolReal::GetUeRrcSapProvider (uint16_t rnti)
{
	NS_LOG_FUNCTION (this);
  std::map<uint16_t, LteUeRrcSapProvider*>::const_iterator it;
  it = m_enbRrcSapProviderMap.find (rnti);
  NS_ASSERT_MSG (it != m_enbRrcSapProviderMap.end (), "could not find RNTI = " << rnti);
  return it->second;
}

void
NrEnbRrcProtocolReal::SetUeRrcSapProvider (uint16_t rnti, LteUeRrcSapProvider* p)
{
	NS_LOG_FUNCTION (this);
  std::map<uint16_t, LteUeRrcSapProvider*>::iterator it;
  it = m_enbRrcSapProviderMap.find (rnti);
  NS_ASSERT_MSG (it != m_enbRrcSapProviderMap.end (), "could not find RNTI = " << rnti);
  it->second = p;
}

void
NrEnbRrcProtocolReal::DoSetupUe (uint16_t rnti, LteEnbRrcSapUser::SetupUeParameters params)
{
  NS_LOG_FUNCTION (this << rnti);

  // // walk list of all nodes to get the peer UE RRC SAP Provider
  /*
   Ptr<LteUeRrc> ueRrc;
   NodeList::Iterator listEnd = NodeList::End ();
  bool found = false;
  for (NodeList::Iterator i = NodeList::Begin (); (i != listEnd) && (found == false); i++)
     {
       Ptr<Node> node = *i;
       int nDevs = node->GetNDevices ();
       for (int j = 0; j < nDevs; j++)
         {
           Ptr<LteUeNetDevice> ueDev = node->GetDevice (j)->GetObject <LteUeNetDevice> ();
           if (!ueDev)
             {
               continue;
             }
           else
             {
               ueRrc = ueDev->GetRrc ();
               if ((ueRrc->GetRnti () == rnti) && (ueRrc->GetCellId () == m_cellId))
                 {
                found = true;
                break;
                 }
             }
         }
     }
   NS_ASSERT_MSG (found , " Unable to find UE with RNTI=" << rnti << " cellId=" << m_cellId);
   m_enbRrcSapProviderMap[rnti] = ueRrc->GetLteUeRrcSapProvider ();
*/
  // just create empty entry, the UeRrcSapProvider will be set by the
  // ue upon connection request or connection reconfiguration
  // completed
  m_enbRrcSapProviderMap[rnti] = 0;

  // Store SetupUeParameters
  m_setupUeParametersMap[rnti] = params;

  LteEnbRrcSapProvider::CompleteSetupUeParameters completeSetupUeParameters;
  std::map<uint16_t, LteEnbRrcSapProvider::CompleteSetupUeParameters>::iterator
    csupIt = m_completeSetupUeParametersMap.find (rnti);
  if (csupIt == m_completeSetupUeParametersMap.end ())
    {
      // Create LteRlcSapUser, LtePdcpSapUser
      LteRlcSapUser* srb0SapUser = new NrRealProtocolRlcSapUser (this,rnti);
      LtePdcpSapUser* srb1SapUser = new LtePdcpSpecificLtePdcpSapUser<NrEnbRrcProtocolReal> (this);
      completeSetupUeParameters.srb0SapUser = srb0SapUser;
      completeSetupUeParameters.srb1SapUser = srb1SapUser;
      // Store LteRlcSapUser, LtePdcpSapUser
      m_completeSetupUeParametersMap[rnti] = completeSetupUeParameters;
    }
  else
    {
      completeSetupUeParameters = csupIt->second;
    }
  m_enbRrcSapProvider->CompleteSetupUe (rnti, completeSetupUeParameters);
}

void
NrEnbRrcProtocolReal::DoRemoveUe (uint16_t rnti)
{
  NS_LOG_FUNCTION (this << rnti);
  std::map<uint16_t, LteEnbRrcSapProvider::CompleteSetupUeParameters>::iterator
    it = m_completeSetupUeParametersMap.find (rnti);
  NS_ASSERT (it != m_completeSetupUeParametersMap.end ());
  delete it->second.srb0SapUser;
  delete it->second.srb1SapUser;
  m_completeSetupUeParametersMap.erase (it);
  m_enbRrcSapProviderMap.erase (rnti);
  m_setupUeParametersMap.erase (rnti);
}

void
NrEnbRrcProtocolReal::DoSendSystemInformation (uint16_t cellId, LteRrcSap::SystemInformation msg)
{
  NS_LOG_FUNCTION (this << cellId);
  // walk list of all nodes to get UEs with this cellId
  Ptr<LteUeRrc> ueRrc;
  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
    {
      Ptr<Node> node = *i;
      int nDevs = node->GetNDevices ();
      for (int j = 0; j < nDevs; ++j)
        {
          Ptr<MmWaveUeNetDevice> ueDev = node->GetDevice (j)->GetObject <MmWaveUeNetDevice> ();
          if (ueDev != 0)
            {
              ueRrc = ueDev->GetRrc ();
              NS_LOG_LOGIC ("considering UE IMSI " << ueDev->GetImsi () << " that has cellId " << ueRrc->GetCellId ());
              if (ueRrc->GetCellId () == cellId)
                {
                  NS_LOG_LOGIC ("sending SI to IMSI " << ueDev->GetImsi ());
                  ueRrc->GetLteUeRrcSapProvider ()->RecvSystemInformation (msg);
                  Simulator::Schedule (RRC_REAL_MSG_DELAY,
                                       &LteUeRrcSapProvider::RecvSystemInformation,
                                       ueRrc->GetLteUeRrcSapProvider (),
                                       msg);
                }
            }
       
        }
    }
}

void
NrEnbRrcProtocolReal::DoSendRrcConnectionSetup (uint16_t rnti, LteRrcSap::RrcConnectionSetup msg)
{
	NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionSetupHeader rrcConnectionSetupHeader;
  rrcConnectionSetupHeader.SetMessage (msg);

/*
  for (std::list<LteRrcSap::DrbToAddMod>::iterator it = msg.radioResourceConfigDedicated.drbToAddModList.begin() ;
		  it != msg.radioResourceConfigDedicated.drbToAddModList.end () ;
		  ++it)
  {
	  it->drbIdentity = 1;
	  it->epsBearerIdentity = 1;
	  it->logicalChannelIdentity = 3;
	  it->logicalChannelConfig.bucketSizeDurationMs = 1000;
	  it->logicalChannelConfig.logicalChannelGroup = 1;
	  it->logicalChannelConfig.prioritizedBitRateKbps = 0;
	  it->logicalChannelConfig.priority = 1;
  }

  msg.rrcTransactionIdentifier = 1;
  msg.radioResourceConfigDedicated.havePhysicalConfigDedicated  = 1;
  msg.radioResourceConfigDedicated.physicalConfigDedicated.haveSoundingRsUlConfigDedicated = 1;
  msg.radioResourceConfigDedicated.physicalConfigDedicated.haveAntennaInfoDedicated = 1;
  msg.radioResourceConfigDedicated.physicalConfigDedicated.havePdschConfigDedicated = 1;
  msg.radioResourceConfigDedicated.physicalConfigDedicated.soundingRsUlConfigDedicated.srsBandwidth = 0;
  msg.radioResourceConfigDedicated.physicalConfigDedicated.soundingRsUlConfigDedicated.srsConfigIndex = 37;
  msg.radioResourceConfigDedicated.physicalConfigDedicated.antennaInfo.transmissionMode = 0;
  msg.radioResourceConfigDedicated.physicalConfigDedicated.pdschConfigDedicated.pa = 4;
*/
  for (std::list<LteRrcSap::DrbToAddMod>::iterator it = msg.radioResourceConfigDedicated.drbToAddModList.begin() ;
		  it != msg.radioResourceConfigDedicated.drbToAddModList.end () ;
		  ++it)
  {
	  NS_LOG_FUNCTION ( "drbIdentity" << it->drbIdentity
	   << "epsBearerIdentity" << it->epsBearerIdentity
	   << "logicalChannelIdentity" << it->logicalChannelIdentity
	   << "choice" << it->rlcConfig.choice
	   << "bucketSizeDurationMs" << it->logicalChannelConfig.bucketSizeDurationMs
	   << "logicalChannelGroup" << it->logicalChannelConfig.logicalChannelGroup
	   << "prioritizedBitRateKbps" << it->logicalChannelConfig.prioritizedBitRateKbps
	   << "priority" << it->logicalChannelConfig.priority );
  }

  NS_LOG_FUNCTION ("rrcTransactionIdentifier " << msg.rrcTransactionIdentifier
                            << " srbToAddModList " << msg.radioResourceConfigDedicated.srbToAddModList.size ()
                            << " drbToAddModList " << msg.radioResourceConfigDedicated.drbToAddModList.size ()
                            << " drbToReleaseList " << msg.radioResourceConfigDedicated.drbToReleaseList.size ()
							<< " havePhysicalConfigDedicated " << msg.radioResourceConfigDedicated.havePhysicalConfigDedicated
							<< " haveSoundingRsUlConfigDedicated " << msg.radioResourceConfigDedicated.physicalConfigDedicated.haveSoundingRsUlConfigDedicated
							<< " haveAntennaInfoDedicated " << msg.radioResourceConfigDedicated.physicalConfigDedicated.haveAntennaInfoDedicated
							<< " havePdschConfigDedicated " << msg.radioResourceConfigDedicated.physicalConfigDedicated.havePdschConfigDedicated
							<< " srsBandwidth " << msg.radioResourceConfigDedicated.physicalConfigDedicated.soundingRsUlConfigDedicated.srsBandwidth
							<< " srsConfigIndex " << msg.radioResourceConfigDedicated.physicalConfigDedicated.soundingRsUlConfigDedicated.srsConfigIndex
							<< " transmissionMode " << msg.radioResourceConfigDedicated.physicalConfigDedicated.antennaInfo.transmissionMode
							<< " pa " << msg.radioResourceConfigDedicated.physicalConfigDedicated.pdschConfigDedicated.pa);

  packet->AddHeader (rrcConnectionSetupHeader);

  LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
  transmitPdcpPduParameters.pdcpPdu = packet;
  transmitPdcpPduParameters.rnti = rnti;
  transmitPdcpPduParameters.lcid = 0;

  m_setupUeParametersMap.at (rnti).srb0SapProvider->TransmitPdcpPdu (transmitPdcpPduParameters);


}

void
NrEnbRrcProtocolReal::DoSendRrcConnectionReject (uint16_t rnti, LteRrcSap::RrcConnectionReject msg)
{
	NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionRejectHeader rrcConnectionRejectHeader;
  rrcConnectionRejectHeader.SetMessage (msg);

  packet->AddHeader (rrcConnectionRejectHeader);

  LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
  transmitPdcpPduParameters.pdcpPdu = packet;
  transmitPdcpPduParameters.rnti = rnti;
  transmitPdcpPduParameters.lcid = 0;

  m_setupUeParametersMap[rnti].srb0SapProvider->TransmitPdcpPdu (transmitPdcpPduParameters);
}


void
NrEnbRrcProtocolReal::DoSendRrcConnectionReconfiguration (uint16_t rnti, LteRrcSap::RrcConnectionReconfiguration msg)
{
	NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReconfigurationHeader rrcConnectionReconfigurationHeader;
  rrcConnectionReconfigurationHeader.SetMessage (msg);

  packet->AddHeader (rrcConnectionReconfigurationHeader);

  LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
  transmitPdcpSduParameters.pdcpSdu = packet;
  transmitPdcpSduParameters.rnti = rnti;
  transmitPdcpSduParameters.lcid = 1;

  m_setupUeParametersMap[rnti].srb1SapProvider->TransmitPdcpSdu (transmitPdcpSduParameters);
}

void
NrEnbRrcProtocolReal::DoSendRrcConnectionReestablishment (uint16_t rnti, LteRrcSap::RrcConnectionReestablishment msg)
{
	NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReestablishmentHeader rrcConnectionReestablishmentHeader;
  rrcConnectionReestablishmentHeader.SetMessage (msg);

  packet->AddHeader (rrcConnectionReestablishmentHeader);

  LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
  transmitPdcpPduParameters.pdcpPdu = packet;
  transmitPdcpPduParameters.rnti = rnti;
  transmitPdcpPduParameters.lcid = 0;

  m_setupUeParametersMap[rnti].srb0SapProvider->TransmitPdcpPdu (transmitPdcpPduParameters);
}


void
NrEnbRrcProtocolReal::DoSendRrcConnectionReestablishmentReject (uint16_t rnti, LteRrcSap::RrcConnectionReestablishmentReject msg)
{
	NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReestablishmentRejectHeader rrcConnectionReestablishmentRejectHeader;
  rrcConnectionReestablishmentRejectHeader.SetMessage (msg);

  packet->AddHeader (rrcConnectionReestablishmentRejectHeader);

  LteRlcSapProvider::TransmitPdcpPduParameters transmitPdcpPduParameters;
  transmitPdcpPduParameters.pdcpPdu = packet;
  transmitPdcpPduParameters.rnti = rnti;
  transmitPdcpPduParameters.lcid = 0;

  m_setupUeParametersMap[rnti].srb0SapProvider->TransmitPdcpPdu (transmitPdcpPduParameters);
}

void
NrEnbRrcProtocolReal::DoSendRrcConnectionRelease (uint16_t rnti, LteRrcSap::RrcConnectionRelease msg)
{
	NS_LOG_FUNCTION (this);
  Ptr<Packet> packet = Create<Packet> ();

  RrcConnectionReleaseHeader rrcConnectionReleaseHeader;
  rrcConnectionReleaseHeader.SetMessage (msg);

  packet->AddHeader (rrcConnectionReleaseHeader);

  LtePdcpSapProvider::TransmitPdcpSduParameters transmitPdcpSduParameters;
  transmitPdcpSduParameters.pdcpSdu = packet;
  transmitPdcpSduParameters.rnti = rnti;
  transmitPdcpSduParameters.lcid = 1;

  m_setupUeParametersMap[rnti].srb1SapProvider->TransmitPdcpSdu (transmitPdcpSduParameters);
}


void
NrEnbRrcProtocolReal::DoReceivePdcpPdu (uint16_t rnti, Ptr<Packet> p)
{
	NS_LOG_FUNCTION (this);
  // Get type of message received
  RrcUlCcchMessage rrcUlCcchMessage;
  p->PeekHeader (rrcUlCcchMessage);

  // Declare possible headers to receive
  RrcConnectionReestablishmentRequestHeader rrcConnectionReestablishmentRequestHeader;
  RrcConnectionRequestHeader rrcConnectionRequestHeader;

  // Deserialize packet and call member recv function with appropriate structure
  switch ( rrcUlCcchMessage.GetMessageType () )
    {
    case 0:
      p->RemoveHeader (rrcConnectionReestablishmentRequestHeader);
      LteRrcSap::RrcConnectionReestablishmentRequest rrcConnectionReestablishmentRequestMsg;
      rrcConnectionReestablishmentRequestMsg = rrcConnectionReestablishmentRequestHeader.GetMessage ();
      m_enbRrcSapProvider->RecvRrcConnectionReestablishmentRequest (rnti,rrcConnectionReestablishmentRequestMsg);
      break;
    case 1:
      p->RemoveHeader (rrcConnectionRequestHeader);
      LteRrcSap::RrcConnectionRequest rrcConnectionRequestMsg;
      rrcConnectionRequestMsg = rrcConnectionRequestHeader.GetMessage ();
      m_enbRrcSapProvider->RecvRrcConnectionRequest (rnti,rrcConnectionRequestMsg);
      break;
    }
}

void
NrEnbRrcProtocolReal::DoReceivePdcpSdu (LtePdcpSapUser::ReceivePdcpSduParameters params)
{
	NS_LOG_FUNCTION (this);
  // Get type of message received
  RrcUlDcchMessage rrcUlDcchMessage;
  params.pdcpSdu->PeekHeader (rrcUlDcchMessage);

  // Declare possible headers to receive
  MeasurementReportHeader measurementReportHeader;
  RrcConnectionReconfigurationCompleteHeader rrcConnectionReconfigurationCompleteHeader;
  RrcConnectionReestablishmentCompleteHeader rrcConnectionReestablishmentCompleteHeader;
  RrcConnectionSetupCompleteHeader rrcConnectionSetupCompleteHeader;

  // Declare possible messages to receive
  LteRrcSap::MeasurementReport measurementReportMsg;
  LteRrcSap::RrcConnectionReconfigurationCompleted rrcConnectionReconfigurationCompleteMsg;
  LteRrcSap::RrcConnectionReestablishmentComplete rrcConnectionReestablishmentCompleteMsg;
  LteRrcSap::RrcConnectionSetupCompleted rrcConnectionSetupCompletedMsg;

  // Deserialize packet and call member recv function with appropriate structure
  switch ( rrcUlDcchMessage.GetMessageType () )
    {
    case 1:
      params.pdcpSdu->RemoveHeader (measurementReportHeader);
      measurementReportMsg = measurementReportHeader.GetMessage ();
      m_enbRrcSapProvider->RecvMeasurementReport (params.rnti,measurementReportMsg);
      break;
    case 2:
      params.pdcpSdu->RemoveHeader (rrcConnectionReconfigurationCompleteHeader);
      rrcConnectionReconfigurationCompleteMsg = rrcConnectionReconfigurationCompleteHeader.GetMessage ();
      m_enbRrcSapProvider->RecvRrcConnectionReconfigurationCompleted (params.rnti,rrcConnectionReconfigurationCompleteMsg);
      break;
    case 3:
      params.pdcpSdu->RemoveHeader (rrcConnectionReestablishmentCompleteHeader);
      rrcConnectionReestablishmentCompleteMsg = rrcConnectionReestablishmentCompleteHeader.GetMessage ();
      m_enbRrcSapProvider->RecvRrcConnectionReestablishmentComplete (params.rnti,rrcConnectionReestablishmentCompleteMsg);
      break;
    case 4:
      params.pdcpSdu->RemoveHeader (rrcConnectionSetupCompleteHeader);
      rrcConnectionSetupCompletedMsg = rrcConnectionSetupCompleteHeader.GetMessage ();
      m_enbRrcSapProvider->RecvRrcConnectionSetupCompleted (params.rnti, rrcConnectionSetupCompletedMsg);
      break;
    }
}

Ptr<Packet>
NrEnbRrcProtocolReal::DoEncodeHandoverPreparationInformation (LteRrcSap::HandoverPreparationInfo msg)
{
	NS_LOG_FUNCTION (this);
  HandoverPreparationInfoHeader h;
  h.SetMessage (msg);

  Ptr<Packet> p = Create<Packet> ();
  p->AddHeader (h);
  return p;
}

LteRrcSap::HandoverPreparationInfo
NrEnbRrcProtocolReal::DoDecodeHandoverPreparationInformation (Ptr<Packet> p)
{
	NS_LOG_FUNCTION (this);
  HandoverPreparationInfoHeader h;
  p->RemoveHeader (h);
  LteRrcSap::HandoverPreparationInfo msg = h.GetMessage ();
  return msg;
}

Ptr<Packet>
NrEnbRrcProtocolReal::DoEncodeHandoverCommand (LteRrcSap::RrcConnectionReconfiguration msg)
{
	NS_LOG_FUNCTION (this);
  RrcConnectionReconfigurationHeader h;
  h.SetMessage (msg);
  Ptr<Packet> p = Create<Packet> ();
  p->AddHeader (h);
  return p;
}

LteRrcSap::RrcConnectionReconfiguration
NrEnbRrcProtocolReal::DoDecodeHandoverCommand (Ptr<Packet> p)
{
	NS_LOG_FUNCTION (this);
  RrcConnectionReconfigurationHeader h;
  p->RemoveHeader (h);
  LteRrcSap::RrcConnectionReconfiguration msg = h.GetMessage ();
  return msg;
}


//////////////////////////////////////////////////////

NrRealProtocolRlcSapUser::NrRealProtocolRlcSapUser (NrEnbRrcProtocolReal* pdcp, uint16_t rnti)
  : m_pdcp (pdcp),
    m_rnti (rnti)
{
	NS_LOG_FUNCTION (this);
}

NrRealProtocolRlcSapUser::NrRealProtocolRlcSapUser ()
{
	NS_LOG_FUNCTION (this);
}

void
NrRealProtocolRlcSapUser::ReceivePdcpPdu (Ptr<Packet> p)
{
	NS_LOG_FUNCTION (this);
  m_pdcp->DoReceivePdcpPdu (m_rnti, p);
}



} // namespace ns3










