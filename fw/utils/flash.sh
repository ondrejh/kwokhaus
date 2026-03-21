if [ ! -d /media/hejdovi/RPI-RP2 ]; then
  stty -F /dev/ttyACM0 1200
  sleep 5.0
fi

#sudo mount /dev/sdb1 /media/vboxuser/RPI-RP2
cp build/*.uf2 /media/hejdovi/RPI-RP2/
#sleep 1.0
#sudo umount /media/vboxuser/RPI-RP2
