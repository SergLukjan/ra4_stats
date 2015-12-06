#!/usr/bin/env python

###### Script to send 60 jobs to the batch finding the limit for each signal mass point
import os, sys, subprocess
import pprint
import glob
import json
import string
import time

# Setting folders
infolder  = "/net/cms2/cms2r0/babymaker/wspaces/"
runfolder = "batch_jobs/" 
if not os.path.exists(runfolder):
  os.system("mkdir -p "+runfolder)

#input datasets
inputfiles = [i for i in os.listdir(infolder) if "xsecNom" in i]

os.system("JobSetup.csh")
njobs = 60
files_job = (len(inputfiles)+njobs-1)/njobs
ifile = 0
ijob = 0
for file in inputfiles:
  ifile += 1
  # Creating executable
  if ifile % files_job == 1:
    ijob += 1
    exename = runfolder+"/wspace_sig_"+str(ijob)+".sh"
    fexe = open(exename,"w")
    os.system("chmod u+x "+exename)
    fexe.write("#!/bin/bash\n\n")
  fexe.write("./run/scan_point.exe -f "+infolder+'/'+file+' >> limits_'+str(ijob)+'.txt\n')
  if ifile % files_job == 0 or ifile == len(inputfiles): 
    fexe.close()
    cmd = "JobSubmit.csh ./run/wrapper.sh ./"+exename
    #print cmd
    os.system(cmd)

print "\nSubmitted "+str(ifile)+" files in "+str(ijob)+" jobs\n"
sys.exit(0)