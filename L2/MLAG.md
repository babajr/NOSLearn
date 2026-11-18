# Multi-Chassis Link Aggregation (MLAG)

## Introduction
MLAG extends link aggregation across two physical devices. A remote device runs a normal LAG,
and the two MLAG peers present themselves to it as one logical LAG partner. 
This gives node-level redundancy: if one device fails, the remote device’s LAG keeps working 
over its links to the surviving peer.

The two peers (primary and secondary) synchronize state with each other to form a single
logical domain called the MCEC (Multi-Chassis Ether Channel) domain.
They exchange this state over the Intra-Domain-Link (IDL) or the Intra-Domain-Peer (IDP).

## Intra-Domain-Link (IDL)
It is used by MLAG primary and secondary devices to synchronize the states between each other
via MCEC packets to create the illusion of a single control plane.
The MLAG peers use the IDL to exchange control information and coordinate with each other
to ensure that data traffic is forwarded properly.

The IDL interface is capable of carrying data traffic:
1. If a MLAG interface to which data traffic is destined is operationally down,
   the data traffic will be sent to MLAG peer device via IDL to avoid any loss.
2. If data traffic received on a MLAG device is destined for a host that is reachable via
   peer MLAG device only, then, the data traffic is sent over IDL to the peer MLAG device.

## Intra-Domain-Peer (IDP)
This is an alternative way to synchronize the states between MLAG primary and secondary devices. 
In particular, it provides a Layer 3 communication path to carry MCEC packets. 
No data traffic is carried over the IDP. 
Hence, data traffic loss is expected, in case of MLAG interface failure.

## IDL vs IDP

| Feature | IDL (Intra-Domain-Link) | IDP (Intra-Domain-Peer) |
| :--- | :--- | :--- |
| **Purpose** | Syncs state between peers using MCEC packets | Alternative sync path for MCEC packets |
| **Layer** | Direct link between the peers | Layer 3 path |
| **Carries data traffic?** | Yes | No, control only |
| **Interconnect** | Dedicated link between peers | The `eth0` out-of-band management interface, or routed in-band over an L3 network |
| **On MLAG interface failure** | Traffic is redirected to the peer over the IDL, so no loss | Traffic is lost, because there is no data path |
