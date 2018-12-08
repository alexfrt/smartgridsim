#!/usr/bin/env python

import xmltodict
import seaborn as sns

with open('outputs/FlowMon.xml') as fd:
    doc = xmltodict.parse(fd.read())

flows = doc['FlowMonitor']['FlowStats']['Flow']

lostPercentage = list()
delayAverages = list()

for flow in flows:
    lostPercentage.append(int(flow[u'@lostPackets']) / int(flow[u'@txPackets']))
    rxPackets = int(flow[u'@rxPackets'])
    if rxPackets > 0:
        delayAverages.append(float(flow[u'@delaySum'][1:-2]) / 1e-6 / int(flow[u'@rxPackets']))
    # flow[u'@jitterSum']

sns.distplot(delayAverages)
