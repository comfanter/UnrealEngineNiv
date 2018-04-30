import os
import sys
import shutil
import glob
import traceback
import fnmatch
import zipfile
import subprocess
import json
from stat import *

def subdir_glob(match):
	path=os.path.dirname(match)
	file=os.path.basename(match)
	fs = [os.path.join(dirpath, f)
		  for dirpath, dirnames, files in os.walk(path)
		  for f in fnmatch.filter(files,file)]
	return fs

sys.tracebacklimit = 5

def EnsureDirectory(dirname,force_lowercase=False):
	if not os.path.exists(dirname):
		if(force_lowercase):
			dirname=dirname.lower()
		os.makedirs(dirname)

def EnsureWritable(dst):
	if not (os.path.exists(dst)):
		return
	try:
		mode = os.stat(dst).st_mode
		dstAtt	=S_IMODE(mode)
		if not (dstAtt & S_IWUSR):
			# File is read-only, so make it writeable
			os.chmod(dst, S_IWUSR)
	except Exception as e:
		exc_type, exc_obj, exc_tb = sys.exc_info()
		for frame in traceback.extract_tb(sys.exc_info()[2]):
			fname,lineno,fn,text = frame
			print("{0} ({1}): error: {2}".format( fname, lineno,exc_type))
		fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
		print('ERROR: Failed to copy to '+dst)

def RunOnWildcard(executable,filename):
	src_dir=os.path.dirname(filename)
	dict= {
			'': '',
	}
	gl=subdir_glob(filename)
	for fl in gl:
		outf=fl.replace(".png",".gnf")
		if (os.path.exists(outf)):
			newtime=os.path.getmtime(fl)
			oldtime=os.path.getmtime(outf)
			#time difference of less than 1s could be down to OS file storage format differences...
			if(newtime-oldtime < 1.0):
				continue
		args=[executable,'-i',fl,'-o',outf,'-f','Auto']
		#print(args)
		pid=subprocess.Popen(args).pid


def CopyWildcard(filename,dest,force_lowercase=False,recurse_subdirs=True,fail_if_none=False,only_newer=False):
	#print("CopyWildcard "+filename+" to "+dest)
	src_dir=os.path.dirname(filename)
	dict= {
			'': '',
	}
	if(recurse_subdirs):
		gl=subdir_glob(filename)
	else:
		gl=glob.glob(filename)
	noneFound=True
	for fl in gl:
		noneFound=False
		fls=os.path.split(fl)
		dest_dir=os.path.join(dest,os.path.relpath(fls[0],src_dir))
		EnsureDirectory(dest_dir,force_lowercase)
		dstf=fls[1]
		if(force_lowercase==True):
			dstf=dstf.lower()
		dst=os.path.normpath(os.path.join(dest_dir,dstf))
		#print(fl+" .. "+dst)
		if (os.path.exists(dst)):
			newtime=os.path.getmtime(fl)
			oldtime=os.path.getmtime(dst)
			#time difference of less than 1s could be down to OS file storage format differences...
			if(only_newer==True and newtime-oldtime < 1.0):
				continue
			#print(dstf+' '+str(newtime-oldtime))
		dict[fl]=dst
	if fail_if_none==True and noneFound==True:
		print("No file found "+filename)
		raise(BaseException)
	numCopied=0
	for fl,dst in dict.items():
		if fl!='':
			EnsureWritable(dst)
			try:
				shutil.copyfile(fl,dst)
				shutil.copystat(fl,dst)
				numCopied=numCopied+1
			except Exception as e:
				exc_type, exc_obj, exc_tb = sys.exc_info()
				for frame in traceback.extract_tb(sys.exc_info()[2]):
					fname,lineno,fn,text = frame
					print("{0} ({1}): error: {2}".format( fname, lineno,exc_type))
				fname = os.path.split(exc_tb.tb_frame.f_code.co_filename)[1]
				print('ERROR: Failed to copy to '+dst)
				if OPTIONAL==True:
					exit (0)
	#if numCopied>0:
	#	print('Copied '+str(numCopied)+' files.')

def ClearWildcard(dest):
	files=subdir_glob(dest)
	for fl in files:
		EnsureWritable(fl)
		if (os.path.exists(fl)):
			try:
				os.remove(fl)
			except Exception as e:
				print("Failed to remove "+fl)
				if OPTIONAL==True:
					exit (0)

Plug=os.getcwd()
			CopyDirectory(TrueSkyPath1,Path.Combine(DeployDirectory,"Engine/Binaries/ThirdParty/Simul/PS4/")		,true,true);
			CopyDirectory(TrueSkyPath2,Path.Combine(DeployDirectory,"Engine/Plugins/TrueSkyPlugin/Resources/")		,true,true);
			CopyDirectory(TrueSkyPath3,Path.Combine(DeployDirectory,"Engine/Plugins/TrueSkyPlugin/Content/")		,true,true);
			CopyDirectory(TrueSkyPath4,Path.Combine(DeployDirectory,"Engine/Plugins/TrueSkyPlugin/shaderbin/PS4")	,true,true);
