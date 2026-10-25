# AnyKernel3 Ramdisk Mod Script
# osm0sis @ xda-developers
# Changes for SkyDragon by HolyAngel @ xda-developers
# Changes for NetHunter by Re4son

## AnyKernel setup
# begin properties
properties() { '
kernel.string=Endfield Kernel 🪐
do.devicecheck=1
do.modules=1
do.systemless=1
do.cleanup=1
do.cleanuponabort=0
device.name1=RMX2180
device.name2=RMX2185
device.name3=Nightmare
supported.versions=
supported.patchlevels=
'; } # end properties

# shell variables
block=/dev/block/by-name/boot;
is_slot_device=0;
ramdisk_compression=auto;

## AnyKernel methods (DO NOT CHANGE)
# import patching functions/variables - see for reference
. tools/ak3-core.sh;

## Trim partitions
fstrim -v /cache;
fstrim -v /data;

## AnyKernel file attributes
# set permissions/ownership for included ramdisk files
chmod -R 750 $ramdisk/*;
chown -R root:root $ramdisk/*;

## AnyKernel install
dump_boot;

# begin ramdisk changes

# init.rc
# backup_file init.rc;
# replace_string init.rc "cpuctl cpu,timer_slack" "mount cgroup none /dev/cpuctl cpu" "mount cgroup none /dev/cpuctl cpu,timer_slack";


# end ramdisk changes

write_boot;
## end install

# Module installation for Wi-Fi & Bluetooth
ui_print " ";
ui_print "Installing Wi-Fi & Bluetooth modules...";

# 1. Mount vendor & system partitions
mount -o rw,remount /vendor 2>/dev/null || mount /vendor 2>/dev/null;
mount -o rw,remount /system 2>/dev/null || mount /system 2>/dev/null;
mount -o rw,remount /system_root 2>/dev/null || mount /system_root 2>/dev/null;

# 2. Inject modules directly to /vendor/lib/modules if writable
if [ -d /vendor/lib/modules ]; then
  ui_print "- Injecting modules to /vendor/lib/modules...";
  cp -fp $home/modules/vendor/lib/modules/*.ko /vendor/lib/modules/ 2>/dev/null;
  chmod 644 /vendor/lib/modules/*.ko 2>/dev/null;
  chown root:root /vendor/lib/modules/*.ko 2>/dev/null;
fi;

if [ -d /system/lib/modules ]; then
  cp -fp $home/modules/system/lib/modules/*.ko /system/lib/modules/ 2>/dev/null;
  chmod 644 /system/lib/modules/*.ko 2>/dev/null;
  chown root:root /system/lib/modules/*.ko 2>/dev/null;
fi;

# 3. KernelSU / Magisk module overlay fallback
# This guarantees that even on Read-Only/EROFS/Dynamic vendor partitions,
# the modules will be automatically mounted over /vendor/lib/modules on boot!
mount -o rw,remount /data 2>/dev/null || mount /data 2>/dev/null;
if [ -d /data/adb/modules ]; then
  ui_print "- Configuring KernelSU/Magisk overlay for Wi-Fi & BT...";
  MODPATH="/data/adb/modules/endfield_wifi_bt_fix";
  rm -rf $MODPATH;
  mkdir -p $MODPATH/system/vendor/lib/modules;
  mkdir -p $MODPATH/system/lib/modules;
  cp -fp $home/modules/vendor/lib/modules/*.ko $MODPATH/system/vendor/lib/modules/ 2>/dev/null;
  cp -fp $home/modules/system/lib/modules/*.ko $MODPATH/system/lib/modules/ 2>/dev/null;
  printf "id=endfield_wifi_bt_fix\nname=Endfield Kernel WiFi & BT Fix\nversion=1.0\nversionCode=1\nauthor=qrevebnation\ndescription=Kernel modules for WiFi & Bluetooth compatibility on custom ROMs\n" > $MODPATH/module.prop
  touch $MODPATH/auto_mount;
  chmod -R 755 $MODPATH;
  chmod 644 $MODPATH/system/vendor/lib/modules/*.ko 2>/dev/null;
  chmod 644 $MODPATH/system/lib/modules/*.ko 2>/dev/null;
  chown -R root:root $MODPATH;
fi;

ui_print "- Wi-Fi & BT module installation complete!";

