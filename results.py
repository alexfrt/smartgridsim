#!/usr/bin/env python

import os
import warnings

import matplotlib.pyplot as plt
import numpy as np
import seaborn as sns
import xmltodict


def main():
    loss_series = list()
    delay_series = list()
    jitter_series = list()

    outputs_dir = 'outputs'
    for subdir in filter(lambda x: x.startswith('trial'), os.listdir(outputs_dir)):
        trial_stats = calculate_trial_statistics("%s/%s" % (outputs_dir, subdir))
        loss_series.append(trial_stats[0])
        delay_series.append(trial_stats[1])
        jitter_series.append(trial_stats[2])

    make_figure_and_print_summary(loss_series, 'loss')
    make_figure_and_print_summary(delay_series, 'delay')
    make_figure_and_print_summary(jitter_series, 'jitter')


def calculate_trial_statistics(directory):
    with open('%s/FlowMon.xml' % directory) as fd:
        doc = xmltodict.parse(fd.read())

    flows = doc['FlowMonitor']['FlowStats']['Flow']

    loss = list()
    delay = list()
    jitter = list()

    for flow in flows:
        loss.append(100.0 * float(flow[u'@lostPackets']) / float(flow[u'@txPackets']))
        if int(flow[u'@rxPackets']) > 1:
            delay.append(float(flow[u'@delaySum'][1:-2]) / 1e6 / int(flow[u'@rxPackets']))
            jitter.append(float(flow[u'@jitterSum'][1:-2]) / 1e6 / int(flow[u'@rxPackets']))

    return np.mean(loss), np.mean(delay), np.mean(jitter)


def make_figure_and_print_summary(series, name):
    try:
        plt.close()
        sns.distplot(series)
        plt.savefig('outputs/%s.svg' % name)
        if 'SHOWPLOT' in os.environ:
            plt.show()
    except Exception as e:
        print("'{}' could not have its plot made due to '{}'".format(name, e.message))

    mean = np.mean(series)
    variance = np.var(series)
    std_deviation = np.std(series)

    print("{:<15} - Mean: {:<7.2f} - StdDev: {:<6.2f} - Variance: {:<8.2f}".format(name, mean, std_deviation, variance))


if __name__ == '__main__':
    warnings.simplefilter(action='ignore', category=FutureWarning)
    main()
