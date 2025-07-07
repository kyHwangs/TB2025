import ROOT
import sys, os
import argparse
from datetime import datetime

print("Start at",datetime.now())
parser = argparse.ArgumentParser()

parser.add_argument('--RunNumber', '-rn', action='store', type=int, required=True, help='Run number')
parser.add_argument('--Config', '-c', action='store', type=str, required=True, help='YAML config file')
parser.add_argument('--MaxEvent', '-me', action='store', type=int, default=-1, help='set maximum event to plot')
parser.add_argument('--MaxFile', '-mf', action='store', type=int, default=-1, help='set maximum event to plot')
parser.add_argument('--fastmode', '-f', action='store_true', default=False, help = 'If true, draw fastmode plot')

args = parser.parse_args()
RunNumber = args.RunNumber
MaxEvent = args.MaxEvent
MaxFile = args.MaxFile
Config = args.Config

if not args.fastmode :
	monit = ROOT.TBmonit('TBwaveform')(Config, RunNumber)
else :
	monit = ROOT.TBmonit('TBfastmode')(Config, RunNumber)

if MaxEvent != -1:
	monit.SetMaxEvent(MaxEvent)

if MaxFile != -1:
	monit.SetMaxFile(MaxFile)

if not args.fastmode :
	monit.Loop()
else :
	monit.LoopFast()

print("Done at",datetime.now())
