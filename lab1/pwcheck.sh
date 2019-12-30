#!/bin/bash
PASSWORD=$1
PASSLEN=$(head -n 1 $PASSWORD)
LENGTH=${#PASSLEN}
# Check password length
if (((LENGTH < 6) || (LENGTH > 32))) ; then
	echo "Error: Password length invalid."
else
	SCORE=$LENGTH
	# Check for special characters
	egrep -q '["#"\$\+%@]' $PASSWORD
	if [ $? -eq 0 ] ; then
		let SCORE=SCORE+5
	fi
	# Check for numbers
	egrep -q '[0-9]' $PASSWORD
	if [ $? -eq 0 ] ; then
	        let SCORE=SCORE+5
	fi
	# Check for letters
	egrep -q '[a-zA-Z]' $PASSWORD
	if [ $? -eq 0 ] ; then
		let SCORE=SCORE+5
	fi
	# Check for consecutive alphanumeric repetition 
	egrep -q "([A-Za-z0-9])\1{1}" $PASSWORD
	if [ $? -eq 0 ] ; then
		let SCORE=SCORE-10
	fi
	# Check for 3 consecutive lowercase characters
	egrep -q '[a-z][a-z][a-z]' $PASSWORD
	if [ $? -eq 0 ] ; then
		let SCORE=SCORE-3
	fi 
	# Check for 3 consecutive uppercase characters
	egrep -q '[A-Z][A-Z][A-Z]' $PASSWORD
	if [ $? -eq 0 ] ; then
		let SCORE=SCORE-3
	fi
	# Check for 3 consecutice numbers
	egrep -q '[0-9][0-9][0-9]' $PASSWORD
	if [ $? -eq 0 ] ; then
		let SCORE=SCORE-3
	fi
	echo "Password Score: $SCORE"
fi
