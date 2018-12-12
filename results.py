#!/usr/bin/env python

import os
import warnings

import matplotlib.pyplot as plt
import numpy as np
import xmltodict
import scipy.stats
from collections import OrderedDict


def calculate_series_statistics(series):
    series = np.array(series)

    mean = np.mean(series)
    variance = np.var(series)
    stddev = np.std(series)
    ci95 = scipy.stats.sem(series) * scipy.stats.t.ppf((1 + 0.95) / 2., len(series)-1)

    return {
        'mean': mean,
        'variance': variance,
        'stddev': stddev,
        'ci95': ci95
    }


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


def calculate_statistics_by_smartmeters_count(directory):
    loss_trials_series = list()
    delay_trials_series = list()
    jitter_trials_series = list()

    for subdir in filter(lambda x: x.startswith('trial'), os.listdir(directory)):
        trial_stats = calculate_trial_statistics("%s/%s" % (directory, subdir))
        loss_trials_series.append(trial_stats[0])
        delay_trials_series.append(trial_stats[1])
        jitter_trials_series.append(trial_stats[2])

    return {
        'loss': calculate_series_statistics(loss_trials_series),
        'delay': calculate_series_statistics(delay_trials_series),
        'jitter': calculate_series_statistics(jitter_trials_series)
    }


def plot_statistics(data, metric):
    labels = list()
    values = list()
    errors = list()

    for n, stats in data.iteritems():
        labels.append(n)
        values.append(stats[metric]['mean'])
        errors.append(stats[metric]['ci95'])

    fig, ax = plt.subplots()
    ax.errorbar(labels, values, yerr=errors, ecolor='red')

    return fig


def main():
    data = dict()

    directory = 'outputs'
    for subdir in filter(lambda x: x.endswith('-meters'), os.listdir(directory)):
        stats = calculate_statistics_by_smartmeters_count("%s/%s" % (directory, subdir))
        numberOfSmartMeters = int(subdir[:-len('-meters')])
        data[numberOfSmartMeters] = stats

    data = OrderedDict(sorted(data.items(), key=lambda x: x[0]))

    plot_statistics(data, 'delay').savefig("%s/%s" % (directory, 'delay.svg'))
    plot_statistics(data, 'jitter').savefig("%s/%s" % (directory, 'jitter.svg'))
    plot_statistics(data, 'loss').savefig("%s/%s" % (directory, 'loss.svg'))


if __name__ == "__main__":
    warnings.simplefilter(action='ignore', category=FutureWarning)
    main()
