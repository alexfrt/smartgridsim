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


def plot_statistics(data, metric, xlabel, ylabel, seriesformat, title):
    fig, ax = plt.subplots()
    
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    
    series_handles = list()
    series_labels = list()
    
    line_styles = ['solid', 'dashed', 'dashdot', 'dotted']
    colors = ['#24d6d6', '#2424d6', '#d6d624']
    i = 1
    
    for series, series_stats in data.iteritems():   
        labels=list()
        values=list()
        errors=list()

        for n, stats in series_stats.iteritems():
            labels.append(n)
            values.append(stats[metric]['mean'])
            errors.append(stats[metric]['ci95'])

        style = line_styles[i%len(line_styles)]
        color = colors[i%len(colors)]
        
        e = ax.errorbar(labels, values, yerr=errors, color=color, ecolor='#d62424', linestyle=style, marker='.', linewidth=3, ms=15, elinewidth=1.5)
        
        series_handles.append(e)
        series_labels.append(seriesformat.format(series))
        
        i += 1
    
    ax.legend(series_handles, series_labels, loc=2)

    return fig


def main():
    dataByAggPercentage = dict()

    basedir = 'outputs'
    for directory in filter(lambda x: x.startswith('agg'), os.listdir(basedir)):
        data = dict()
        for subdir in filter(lambda x: x.endswith('-meters'), os.listdir("%s/%s" % (basedir, directory))):
            stats = calculate_statistics_by_smartmeters_count("%s/%s/%s" % (basedir, directory, subdir))
            numberOfSmartMeters = int(subdir[:-len('-meters')])
            data[numberOfSmartMeters] = stats
        data = OrderedDict(sorted(data.items(), key=lambda x: x[0]))
        dataByAggPercentage[int(directory[3:])] = data

    plot_statistics(dataByAggPercentage, 'delay', '# of Smart Meters', 'Delay (ms)', '{}% aggregation', 'Perceived delay').savefig("%s/%s" % (basedir, 'delay.svg'))
    plot_statistics(dataByAggPercentage, 'jitter', '# of Smart Meters', 'Jitter (ms)', '{}% aggregation', 'Perceived jitter').savefig("%s/%s" % (basedir, 'jitter.svg'))
    plot_statistics(dataByAggPercentage, 'loss', '# of Smart Meters', 'Loss %', '{}% aggregation', 'Perceived loss').savefig("%s/%s" % (basedir, 'loss.svg'))


if __name__ == "__main__":
    warnings.simplefilter(action='ignore', category=FutureWarning)
    main()
