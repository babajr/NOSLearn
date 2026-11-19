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

## Topology
                       +-------------------+
                       | Upstream network  |
                       +-------------------+
                                 |
                  +--------------+--------------+
                  |                             |
      +-----------+-----------------------------+-----------+
      |           |  MCEC domain (single node)  |           |
      |           |                             |           |
      |     +-----------+                 +-----------+     |
      |     |  Peer A   | ===== IDL ===== |  Peer B   |     |
      |     |  Primary  | - - - IDP - - - | Secondary |     |
      |     +-----------+                 +-----------+     |
      |           |                             |           |
      +-----------+-----------------------------+-----------+
                  |                             |
                  |      one LAG (2 links)      |
                  |                             |
              +-------------------------------------+
              |            Remote device            |
              |          Runs standard LAG          |
              +-------------------------------------+

## MLAG Modes
MLAG can be configured in MLAG Active-Active or MLAG Active-Standby modes.
Both modes use the same building blocks: two peers, one logical LAG endpoint, and state sync between them.
The difference is which peer’s links carry traffic at the same time.

### MLAG Active-Active
Both peers forward traffic simultaneously. The remote device sees one LACP partner and hashes flows across all member links,
so some flows go to peer A and some to peer B.
#### Bandwidth: 
All links are used, with no idle capacity.
#### Failover:
If one peer or its links fail, the remote LAG just stops hashing onto those links. Convergence is very fast, often sub-second.
#### Requirements:
The peers must synchronize MAC tables (and ARP/ND for L3 designs) and share a common LACP system ID. 
They also need split-brain protection, and the peer link must be sized for traffic that arrives on one peer but exits through the other.
#### Typical use:
Data center L2 and L3 designs (Arista MLAG, Cisco vPC, Cumulus), usually paired with VRRP or an anycast gateway.
#### Topology
          +--------------------+
          |  Upstream network  |
          +--------------------+
             ||            ||
        +----------+   +----------+
        | Peer A   |<->| Peer B   |
        |Forwarding|IDL|Forwarding|
        +----------+   +----------+
             ||            ||
           Active        Active
              \\          //
          +--------------------+
          |   Remote device    |
          | Hashes flows to    |
          |       both         |
          +--------------------+

### MLAG Active-Standby
Only one peer’s links actively carry traffic for a given LAG or service. The other peer’s links are up but held in standby
(for example, in an LACP standby or out-of-sync state) until the active side fails.
#### Bandwidth:
Only the active links forward, so standby capacity sits idle.
#### Failover:
The standby peer takes over when the active peer or links fail. 
Convergence is usually slower than active-active, because LACP has to re-sync and the standby has to bring its links into forwarding.
#### Simplicity:
Since only one side forwards at a time, there is less risk of loops, duplicate frames, and MAC-flapping. State sync is also simpler.
#### Typical use:
Service-provider designs with VPWS and pseudowire redundancy, such as ICCP-based MC-LAG, where deterministic behavior matters more than using every link.
#### Topology
          +--------------------+
          |  Upstream network  |
          +--------------------+
             ||            :
        +----------+   +- - - - - -+
        | Peer A   |<=>| Peer B    |
        | Active   |IDL| Standby   |
        +----------+   +- - - - - -+
             ||            :
           Active       Standby
              \\          :
          +--------------------+
          |   Remote device    |
          | Uses active link   |
          |       only         |
          +--------------------+
