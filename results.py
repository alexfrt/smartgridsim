#!/usr/bin/env python

import os
import xmltodict
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import describe


def main():
    with open('outputs/FlowMon.xml') as fd:
        doc = xmltodict.parse(fd.read())

    flows = doc['FlowMonitor']['FlowStats']['Flow']

    lost_percentages = list()
    delay_averages = list()
    jitter_averages = list()

    for flow in flows:
        lost_percentages.append(100.0 * float(flow[u'@lostPackets']) / float(flow[u'@txPackets']))
        if int(flow[u'@rxPackets']) > 1:
            delay_averages.append(float(flow[u'@delaySum'][1:-2]) / 1e6 / int(flow[u'@rxPackets']))
            jitter_averages.append(float(flow[u'@jitterSum'][1:-2]) / 1e6 / int(flow[u'@rxPackets']))

    make_figure_and_print_summary(lost_percentages, 'lostPercentages')
    make_figure_and_print_summary(delay_averages, 'delayAverages')
    make_figure_and_print_summary(jitter_averages, 'jitterAverages')


def make_figure_and_print_summary(series, name):
    sns.distplot(series)
    plt.savefig('outputs/%s.svg' % name)
    if 'SHOWPLOT' in os.environ:
        plt.show()

    stats = describe(series)
    mean = getattr(stats, 'mean')
    variance = getattr(stats, 'variance')
    std_deviation = np.sqrt(variance)

    print("{:<15} - Mean: {:<6.2f} - StdDev: {:<6.2f} - Variance: {:<8.2f}".format(name, mean, std_deviation, variance))


if __name__ == '__main__':
    main()
