#!/bin/sh

echo "\033[1;32mDisk Cleaup\033[0m"
df -h | grep 'Users' | awk ' { printf("\n>> Current Disk Usage : %s\n\n" , $5) }' 

read -p "Do you want to remove unnecessary files? [y/n] : " answer

if [ $answer = y ] || [ $answer = Y ]
then
	# delete large cache folder in Library/Caches
	rm -rf ~/Library/Caches/vscode-cpptools
	rm -rf ~/Library/Caches/com.microsoft.VS*
	rm -rf ~/Library/Caches/com.google.SoftwareUpdate
	rm -rf ~/Library/Caches/Google
	rm -rf ~/Library/Caches/com.apple.Music
	rm -rf ~/Library/Caches/com.apple.iTunes
	echo "\033[1;33mFiles from Library/Caches deleted\033[0m"
	# delete Slack Caches (IndexedDB and CacheStorage)
	rm -rf ~/Library/Application\ Support/Slack/IndexedDB/*
	rm -rf ~/Library/Application\ Support/Slack/Service\ Worker/CacheStorage/*
	echo "\033[1;33mFiles from Library/Application Support/Slack deleted\033[0m"
	df -h | grep 'Users' | awk ' { printf("\n>> Result : %s\n\n" , $5) }' 
	echo "\033[1;32mCleanup Finished.\033[0m"
	echo ''
else
	echo "\033[0;31mUser Cancelled Cleanup\033[0m"
fi
