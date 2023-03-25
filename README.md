# Audi A3 8P Red FIS Project
====================
<b>This is a project about discovering (reverse-engineering) Audi A3 8P dashboard's Driver Information System and its protocol.</b>

For people who don't know what is DIS or FIS:
https://www.youtube.com/watch?v=bKFak5AGAGo

The goal is to implement a similiar but simpler open-source alternative to the "FIS-Control" or "Polar FIS".<br>
https://fis-control.de/index_en.html<br>
https://www.urotuning.com/products/polar-fis-advanced-multi-function-display-pf-04?variant=8384889323575

<b> Picture about the current status:</b><br>
<img src="https://github.com/tothpetiszilard/a3_8p_redFIS/blob/main/media/alive.png" alt="picture">

## Compatibility (from version 0.2):
Selectable in "menuconfig":
- You can use Alternative VWTP CANID if you have RNS-E (Audi navigation with color display) installed and your dashboards part number is "8P0 920 930". Don't select "FIS-Control" in your RNS-E custom firmware in this case. Enter/exit FIS scenarios are a bit buggy.

- You have to use Navigation VWTP CANID if you have dashboard with part number "8P0 920 931". Alternative CANID is not supported by the dash. You may select "FIS-Control" in your RNS-E custom firmware in this case, but handling this is not yet implemented.

<i>Brave volunteers are warmly welcomed for testing... :)</i>

Used and helpful sources:<br>
https://docs.google.com/spreadsheets/d/1eirT8LbSRl4j06BpwgsiE4PM_2BGH9UStdWLXwKvHJw/edit#gid=10<br>
https://i-wiki.tech/?post=vw-transport-protocol-20-tp-20-for-can-bus<br>
http://www.orlaus.dk/mercantec/UdvidetHardwareSoftware/Can/OBD2/VW%20Transport%20Protocol%202.0%20(TP%202.0)%20for%20CAN%20bus%20_%20jazdw.pdf<br>
https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3/blob/master/vehicle/OVMS.V3/components/vehicle/docs/VW-TP-2.0.txt<br>

