#!/bin/bash

YELLOW='\033[1;33m'
NC='\033[0m'

TMP_FOLDER=$(mktemp -d)
CONFIG_FILE='kyd.conf'
CONFIGFOLDER='/root/.kydcore'
COIN_DAEMON='kydd'
COIN_CLI='kyd-cli'
COIN_PATH='/usr/local/bin/'
if [[ $(lsb_release -d) = *16.04* ]]; then
COIN_TGZ='https://github.com/kydcoin/KYD3/releases/download/v3.2.0.3/kyd-3.2.0.3-Ubuntu16-x86_64.tar.gz'
echo $COIN_TGZ
fi
if [[ $(lsb_release -d) = *18.04* ]]; then
COIN_TGZ='https://github.com/kydcoin/KYD3/releases/download/v3.2.0.3/kyd-3.2.0.3-Ubuntu18-x86_64.tar.gz'
fi
COIN_BOOTSTRAP='https://review.kydcoin.io/bootstrap/kyd-bootstrap.zip'
BOOTSTRAP_ZIP=$(echo $COIN_BOOTSTRAP | awk -F'/' '{print $NF}')
COIN_ZIP=$(echo $COIN_TGZ | awk -F'/' '{print $NF}')
COIN_NAME='kyd'
COIN_PORT=12244
RPC_PORT=12243

NODEIP=$(curl -s4 icanhazip.com)

RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

function download_node() {
  echo -e "Preparing to download updated $COIN_NAME binaries..."
  cd $TMP_FOLDER
  wget -q $COIN_TGZ
  tar xvzf $COIN_ZIP -C /usr/local/bin/ --strip=1
  chmod +x $COIN_PATH$COIN_DAEMON $COIN_PATH$COIN_CLI
  cd - >/dev/null 2>&1
  rm -r $TMP_FOLDER >/dev/null 2>&1
}

function configure_systemd() {
  cat << EOF > /etc/systemd/system/$COIN_NAME.service
[Unit]
Description=$COIN_NAME service
After=network.target
[Service]
User=root
Group=root
Type=forking
#PIDFile=$CONFIGFOLDER/$COIN_NAME.pid
ExecStart=$COIN_PATH$COIN_DAEMON -daemon -conf=$CONFIGFOLDER/$CONFIG_FILE -datadir=$CONFIGFOLDER
ExecStop=-$COIN_PATH$COIN_CLI -conf=$CONFIGFOLDER/$CONFIG_FILE -datadir=$CONFIGFOLDER stop
Restart=always
PrivateTmp=true
TimeoutStopSec=60s
TimeoutStartSec=10s
StartLimitInterval=120s
StartLimitBurst=5
[Install]
WantedBy=multi-user.target
EOF
resync_bootstrap
  systemctl daemon-reload
  sleep 3
  systemctl start $COIN_NAME.service
  systemctl enable $COIN_NAME.service >/dev/null 2>&1

  if [[ -z "$(ps axo cmd:100 | egrep $COIN_DAEMON)" ]]; then
    echo -e "${RED}$COIN_NAME is not running${NC}, please investigate. You should start by running the following commands as root:"
    echo -e "${GREEN}systemctl start $COIN_NAME.service"
    echo -e "systemctl status $COIN_NAME.service"
    echo -e "less /var/log/syslog${NC}"
    exit 1
  fi
}


function create_config() {
  mkdir $CONFIGFOLDER >/dev/null 2>&1
  RPCUSER=$(tr -cd '[:alnum:]' < /dev/urandom | fold -w10 | head -n1)
  RPCPASSWORD=$(tr -cd '[:alnum:]' < /dev/urandom | fold -w22 | head -n1)
  cat << EOF > $CONFIGFOLDER/$CONFIG_FILE
rpcuser=$RPCUSER
rpcpassword=$RPCPASSWORD
rpcport=$RPC_PORT
rpcallowip=127.0.0.1
listen=1
server=1
daemon=1
port=$COIN_PORT
EOF
}

function create_key() {
  echo -e "Enter your ${RED}$COIN_NAME Masternode Private Key${NC}. Leave it blank to generate a new ${RED}Masternode Private Key${NC} for you:"
  read -e COINKEY
  if [[ -z "$COINKEY" ]]; then
  $COIN_PATH$COIN_DAEMON -daemon
  sleep 30
  if [ -z "$(ps axo cmd:100 | grep $COIN_DAEMON)" ]; then
   echo -e "${RED}$COIN_NAME server couldn not start. Check /var/log/syslog for errors.{$NC}"
   exit 1
  fi
  COINKEY=$($COIN_PATH$COIN_CLI masternode genkey)
  if [ "$?" -gt "0" ];
    then
    echo -e "${RED}Wallet not fully loaded. Let us wait and try again to generate the Private Key${NC}"
    sleep 30
    COINKEY=$($COIN_PATH$COIN_CLI masternode genkey)
  fi
  $COIN_PATH$COIN_CLI stop
fi
clear
}

function update_config() {
  #sed -i 's/daemon=1/daemon=0/' $CONFIGFOLDER/$CONFIG_FILE
  cat << EOF >> $CONFIGFOLDER/$CONFIG_FILE
logintimestamps=1
maxconnections=256
#bind=$NODEIP
masternode=1
externalip=$NODEIP:$COIN_PORT
masternodeprivkey=$COINKEY
EOF
}


function enable_firewall() {
  echo -e "Installing and setting up firewall to allow ingress on port ${GREEN}$COIN_PORT${NC}"
  ufw allow $COIN_PORT/tcp comment "$COIN_NAME MN port" >/dev/null
  ufw allow ssh comment "SSH" >/dev/null 2>&1
  ufw limit ssh/tcp >/dev/null 2>&1
  ufw default allow outgoing >/dev/null 2>&1
  echo "y" | ufw enable >/dev/null 2>&1
}



function get_ip() {
  declare -a NODE_kyd
  for kyd in $(netstat -i | awk '!/Kernel|Iface|lo/ {print $1," "}')
  do
    NODE_kyd+=($(curl --interface $kyd --connect-timeout 2 -s4 icanhazip.com))
  done

  if [ ${#NODE_kyd[@]} -gt 1 ]
    then
      echo -e "${GREEN}More than one IP. Please type 0 to use the first IP, 1 for the second and so on...${NC}"
      INDEX=0
      for ip in "${NODE_kyd[@]}"
      do
        echo ${INDEX} $ip
        let INDEX=${INDEX}+1
      done
      read -e choose_ip
      NODEIP=${NODE_kyd[$choose_ip]}
  else
    NODEIP=${NODE_kyd[0]}
  fi
}


function compile_error() {
if [ "$?" -gt "0" ];
 then
  echo -e "${RED}Failed to compile $COIN_NAME. Please investigate.${NC}"
  exit 1
fi
}


function checks() {

if [[ $EUID -ne 0 ]]; then
   echo -e "${RED}$0 must be run as root.${NC}"
   exit 1
fi

if [ -n "$(pidof $COIN_DAEMON)" ] || [ -e "$COIN_DAEMOM" ] ; then
  echo -e "${RED}$COIN_NAME is already installed.${NC}"
  exit 1
fi
}

function prepare_system() {
echo -e "Preparing the system to install ${GREEN}$COIN_NAME${NC} masternode."
apt-get update >/dev/null 2>&1
DEBIAN_FRONTEND=noninteractive apt-get update > /dev/null 2>&1
DEBIAN_FRONTEND=noninteractive apt-get -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" -y -qq upgrade >/dev/null 2>&1
apt install -y software-properties-common >/dev/null 2>&1
echo -e "${GREEN}Adding bitcoin PPA repository"
apt-add-repository -y ppa:bitcoin/bitcoin >/dev/null 2>&1
echo -e "Installing required packages, it may take some time to finish.${NC}"
apt-get update >/dev/null 2>&1
apt-get install -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold" make software-properties-common \
build-essential libtool autoconf libssl-dev libboost-dev libboost-chrono-dev libboost-filesystem-dev libboost-program-options-dev \
libboost-system-dev libboost-test-dev libboost-thread-dev sudo automake git wget curl libdb4.8-dev bsdmainutils libdb4.8++-dev \
libminiupnpc-dev libgmp3-dev ufw pkg-config libevent-dev  libdb5.3++ libzmq5 >/dev/null 2>&1
if [ "$?" -gt "0" ];
  then
    echo -e "${RED}Not all required packages were installed properly. Try to install them manually by running the following commands:${NC}\n"
    echo "apt-get update"
    echo "apt -y install software-properties-common"
    echo "apt-add-repository -y ppa:bitcoin/bitcoin"
    echo "apt-get update"
    echo "apt install -y make build-essential libtool software-properties-common autoconf libssl-dev libboost-dev libboost-chrono-dev libboost-filesystem-dev \
libboost-program-options-dev libboost-system-dev libboost-test-dev libboost-thread-dev sudo automake git curl libdb4.8-dev \
bsdmainutils libdb4.8++-dev libminiupnpc-dev libgmp3-dev ufw fail2ban pkg-config libevent-dev libzmq5"
 exit 1
fi

clear
}

function create_swap() {
 echo -e "Checking if swap space is needed."
 PHYMEM=$(free -g|awk '/^Mem:/{print $2}')
 SWAP=$(free -g|awk '/^Swap:/{print $2}')
 if [ "$PHYMEM" -lt "1" ] && [ -n "$SWAP" ]
  then
    echo -e "${GREEN}Server is running with less than 1G of RAM without SWAP, creating 2G swap file.${NC}"
    SWAPFILE=$(mktemp)
    dd if=/dev/zero of=$SWAPFILE bs=1024 count=2M
    chmod 600 $SWAPFILE
    mkswap $SWAPFILE
    swapon -a $SWAPFILE
 else
  echo -e "${GREEN}Server running with at least 1G of RAM, no swap needed.${NC}"
 fi
 clear
}



function important_information() {
 echo
 echo -e "================================================================================================================================"
 echo -e "$COIN_NAME Masternode is up and running listening on port ${RED}$COIN_PORT${NC}."
 echo -e "Configuration file is: ${RED}$CONFIGFOLDER/$CONFIG_FILE${NC}"
 echo -e "Start: ${RED}systemctl start $COIN_NAME.service${NC}"
 echo -e "Stop: ${RED}systemctl stop $COIN_NAME.service${NC}"
 echo -e "VPS_IP:PORT ${RED}$NODEIP:$COIN_PORT${NC}"
 echo -e "MASTERNODE PRIVATEKEY is: ${RED}$COINKEY${NC}"
 echo -e "Please check ${RED}$COIN_NAME${NC} is running with the following command: ${RED}systemctl status $COIN_NAME.service${NC}"
 echo -e "Please check that your chain is fully synced before starting it from local wallet. To do this type kyd-cli mnsync status. ${GREEN}RequestedMasternodeAssets${NC} must equal ${GREEN}999${NC}"
 echo -e "================================================================================================================================"
}

function setup_node() {
  get_ip
  create_config
  create_key
  update_config
  enable_firewall
  configure_systemd
  important_information
  
exit 1
}
function user_input() {
YELLOW='\033[1;33m'
NC='\033[0m'
    NORMAL=`echo "\033[m"`
    MENU=`echo "\033[36m"` #Blue
    NUMBER=`echo "\033[33m"` #yellow
    FGRED=`echo "\033[41m"`
    RED_TEXT=`echo "\033[31m"`
    ENTER_LINE=`echo "\033[33m"`

    echo -e "${MENU}*********************************************${NORMAL}"
    echo -e "${MENU}****Welcome to the KYD Masternode setup******${NORMAL}"
    echo -e "${MENU}*********************************************${NORMAL}"
    echo -e "${MENU}**${NUMBER} 1)${MENU} Install New Masternode               **${NORMAL}"
    echo -e "${MENU}**${NUMBER} 2)${MENU} Update Node                          **${NORMAL}"
    echo -e "${MENU}**${NUMBER} 3)${MENU} Resync with Bootstrap                **${NORMAL}"
    echo -e "${MENU}**${NUMBER} 4)${MENU} Resync without Bootstrap             **${NORMAL}"
    echo -e "${MENU}**${NUMBER} 5)${MENU} Exit                                 **${NORMAL}"
    echo -e "${MENU}*********************************************${NORMAL}"
    echo -e "${ENTER_LINE}Enter option and press enter. ${NORMAL}"
    read opt </dev/tty
    menu_loop

}

function menu_loop() {

while [ opt != '' ]
    do
        case $opt in
        1)newInstall;
        ;;
        2)UpdateNode;
        ;;
        3)resync_bootstrap;
        ;;
        4)resync_no_bootstrap;
        ;;
        5)echo -e "Exiting...";sleep 1;exit 0;
        ;;
        \n)exit 0;
        ;;
        *)clear;
        "Pick an option from the menu";
        user_input;
        ;;
    esac
done
}

function resync_no_bootstrap() {
echo -e "${RED}Stopping $COIN_DAEMON...${NC}"
systemctl stop $COIN_NAME.service
echo -e "${RED}Sleeping for 20 seconds...${NC}";
sleep 20
cd $CONFIGFOLDER
echo -e "${YELLOW}Clearing existing files...${NC}"
mv wallet{.dat,.keep}
mv ${COIN_NAME,,}{.conf,.keep}
rm -rf *.conf *.dat *.log blocks chainstate backups database sporks .lock -r
mv wallet{.keep,.dat}
mv ${COIN_NAME,,}{.keep,.conf}
ls -al
sleep 5
echo -e "${YELLOW}Starting $COIN_DAEMON...${NC}"
systemctl start $COIN_NAME.service
echo -e "${GREEN}$COIN_NAME Masternode refreshed!${NC}"
exit 1
}

function resync_bootstrap() {
echo -e "${RED}Stopping $COIN_DAEMON...${NC}"
systemctl stop $COIN_NAME.service
echo -e "${RED}Sleeping for 20 seconds...${NC}";
sleep 20
cd $CONFIGFOLDER
echo -e "${YELLOW}Clearing existing files...${NC}"
mv wallet{.dat,.keep}
mv ${COIN_NAME,,}{.conf,.keep}
rm -rf *.conf *.dat *.log blocks chainstate database sporks zerocoin .lock -r
mv wallet{.keep,.dat}
mv ${COIN_NAME,,}{.keep,.conf}
ls -al
sleep 1
download_bootstrap
sleep 1
echo -e "${YELLOW}Starting $COIN_DAEMON...${NC}"
systemctl start $COIN_NAME.service
echo -e "${GREEN}${COIN_NAME^^} Masternode refreshed!${NC}"
exit 1
}

function download_bootstrap() {
cd $CONFIGFOLDER
apt-get -qq install unzip
echo -e "Downloading Bootstrap"
wget -q $COIN_BOOTSTRAP
unzip -qo $BOOTSTRAP_ZIP
rm $BOOTSTRAP_ZIP
}

function newInstall() {
checks
prepare_system
create_swap
download_node
setup_node
exit 1
}

function UpdateNode() {
echo -e "Stopping KYD Service"
cp /etc/systemd/system/KYD.service /etc/systemd/system/kyd.service > /dev/null 2>&1
systemctl stop $COIN_NAME.service > /dev/null 2>&1
systemctl stop KYD.service > /dev/null 2>&1
rm /etc/systemd/system/KYD.service > /dev/null 2>&1
download_node
echo -e "Restarting Node"
  systemctl start $COIN_NAME.service
  echo -e "${GREEN}$COIN_NAME Masternode has been updated!${NC}"
exit 1
}


##### Main #####
clear
user_input
