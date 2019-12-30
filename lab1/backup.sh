#!/bin/bash

# Counter for # of backups 
BKUPCOUNT=0

# Get current Date and make new file name
DATE=$(date +"%Y-%m-%d-%H-%M-%S")
newName="$DATE-$1"
# Move a copy of new File with newName into the backup directory
cp -a $1 $newName
mv $newName $2

# Do this indefinatly
while true; do
	# Sleep for time specified by input
	sleep $3
	# Get location of newest backup
	BACKUPFILELOC="$2/$newName"
	# Check if the counter is less than or equal to paramater 4
	if [ $BKUPCOUNT -le $4 ]; then
		# Call diff on newest backup and current file
		diff $1 $BACKUPFILELOC > diff-message
		# If different
		if [ $? -ne 0 ]; then
			# Increment backup counter
			let BKUPCOUNT=BKUPCOUNT+1
			# Get current Date and create new file
			DATE=$(date +"%Y-%m-%d-%H-%M-%S")
			newName="$DATE-$1"
			# Move a copy of the new file into the backup folder
			cp -a $1 $newName
			mv $newName $2
			# If the total number of backups created is greater that the limit 
			if [ $BKUPCOUNT -ge $4 ]; then 
				# Get the oldest file and remove it
				OLDEST=$(ls -t $2 | tail -n 1)
				rm $2/$OLDEST
				let BKUPCOUNT=BKUPCOUNT-1
			fi
			# Send the user the diff output to ther purdue.edu email
			/usr/bin/mailx -s "There have been changes to you file $1" "$USER@purdue.edu" < diff-message
		fi
	fi
done
