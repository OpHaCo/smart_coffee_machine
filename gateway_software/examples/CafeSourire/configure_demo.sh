#!/bin/sh

# configure coffee machine demo - 2 modes
# * a4h - demo in a4h creativity lab with a4h team and VIP recognition. Accesspoint AND wlan1 connected to creativity_lab network
# * outside - Only VIP recognition. No wlan1 not connected to creativity_lab network

usage="configure_demo [demo_mode] 
demo_mode : * 'a4h'              - a4h - demo in a4h creativity lab with a4h team and VIP recognition. Accesspoint AND wlan1 connected to creativity_lab network
            * 'outside'(default) - Only VIP recognition. No wlan1 not connected to creativity_lab network"
mode=
if [ "$#" -eq 1 ] 
then
	if [ "$1" == 'a4h' ] || [ "$1" == 'outside' ]
	then
		mode="$1"	
	else
		echo "Invalid parameter given $1 - Usage :"
		echo "$usage"
		exit 1
	fi
elif [ "$#" -eq 0 ]
then
	mode='outside'
else
		echo "Invalid number of parameters given - Usage :"
		echo "$usage"
		exit 1
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ "$mode" == 'a4h' ]
then
	echo 'a4h mode'
	# enable wlan1 automatic connection to creativity lab
	if iwconfig 2>/dev/null |grep -q 'wlan1'
	then
		echo 'wlan1 active'
		if systemctl list-units --type=service | grep -q 'netctl-auto@wlan1.service'
		then
			echo 'wlan1 automatic connection to creativity_lab already started'
		else
			echo 'start wlan1 automatic connection to creativity_lab already started'
			systemctl start netctl-auto@wlan1.service	
		fi	
		#be sure it is enabled
		systemctl enable netctl-auto@wlan1.service
	else
		echo 'wlan1 not active'
	fi

	#add a4h_team + VIP face recognition
	if ls -l $DIR/training_set >/dev/null 2>&1 | grep -q 'training_set_a4h'
	then
		echo 'a4h training set already used'
	else
		echo 'set a4h training set'
		ln -nfs $DIR/training_set_a4h $DIR/training_set
	fi
else
	echo 'outside mode'
	if systemctl list-units --type=service | grep -q 'netctl-auto@wlan1.service'
	then
		# disable wlan1 automatic connection to creativity lab
		echo 'disable wlan1 automatic connection'
		#systemctl stop netctl-auto@wlan1.service	
	fi
	# disable wlan1 automatic connection to creativity lab
	systemctl disable netctl-auto@wlan1.service	
	#add VIP face recognition 
	if ls -l $DIR/training_set |grep -q 'training_set_demo'
	then
		echo 'demo training set already used'
	else
		echo 'set demo training set'
		ln -nfs $DIR/training_set_demo $DIR/training_set
	fi
fi
