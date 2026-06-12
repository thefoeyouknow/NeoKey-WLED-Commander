# NeoKey WLED Commander — User Guide

Welcome to the NeoKey WLED Commander! This device allows you to effortlessly control multiple WLED instances on your network simultaneously, mapping physical mechanical keys to WLED presets with instantaneous feedback.

## 1. Initial Setup
When you first boot up the NeoKey WLED Commander, it needs to be configured with your local network settings.

1. Power the NeoKey device via USB-C.
2. The device will create a temporary Wi-Fi network (Access Point) called **WLED-Key-Setup**.
3. Connect your phone or computer to this network.
4. If the captive portal does not automatically open, open a web browser and navigate to `http://192.168.4.1`.
5. On the **Network** tab, enter your home Wi-Fi `SSID` and `Password`.
6. Click **Save Settings** at the bottom of the page. The device will reboot and join your home network.

## 2. Accessing the Dashboard
Once the device is on your home network, it can be accessed via a web browser:
- **Windows/Mac/iOS:** Navigate to `http://wled-commander.local`
- Alternatively, you can find the device's IP address from your router's client list and navigate to that IP.

## 3. Adding WLED Devices
The NeoKey can control any number of WLED devices simultaneously.

1. Open the Web Dashboard.
2. Navigate to the **Network Configuration** tab.
3. Under the **WLED Devices** section, you can either:
   - Click **Scan Local Network** to use auto-discovery. Click **Add** on any WLED instances you want to control.
   - Click **+ Add IP Manually** and type in the IP address or mDNS hostname of your WLED devices.
4. Click **Save Settings** to apply your changes.

## 4. Visual Key Mapper
The visual key mapper allows you to assign specific WLED presets to your physical mechanical keys.

1. Navigate to the **Key Mapper** tab on the dashboard.
2. Click on the physical key you want to edit.
3. **Auto-Map Feature:**
   - By default, the device is set to **"Automatically assign the 4 lowest WLED presets"**. 
   - This means it will reach out to your primary WLED device, find the 4 lowest preset IDs, and automatically assign them to keys 0, 1, 2, and 3.
4. **Manual Override:**
   - Uncheck the Auto-Map box if you want to manually specify the preset IDs.
   - In the editor panel below, type the **WLED Preset ID** you want the key to activate.
5. **Custom Labels:**
   - You can enter a **Custom Label** (e.g., "Sunrise", "Cyberpunk", "Red").
   - This label will appear on the blue badge on the key in the dashboard UI.
   - *Note:* If Auto-Map is enabled and you leave the label blank, it will automatically pull the preset's actual name from WLED!

## 5. System Settings
Under the **System Settings** tab, you can customize the hardware behavior.

- **Active Key Brightness:** The brightness of the LED beneath the key when the preset is currently active.
- **Inactive Key Brightness:** The background brightness of the LEDs when they are standing by.
- **LED Feedback Style:**
  - **Solid:** LEDs snap to the new color instantly.
  - **Fade:** LEDs smoothly transition colors when a new preset is selected.
  - **Flash:** The pressed key pulses white momentarily before settling on the new color to confirm your press.

## 6. Over-The-Air (OTA) Updates
You can update the firmware of your NeoKey WLED Commander without plugging it into a computer.

1. Download the latest `firmware.bin` from the GitHub Releases page.
2. Navigate to the **System Settings** tab in the dashboard.
3. Under **OTA Firmware Update**, select the `.bin` file and click **Flash Firmware**.
4. Do not remove power from the device while the progress bar is filling.

## 7. Factory Reset
If you ever need to completely wipe the device settings (e.g., you changed your Wi-Fi router or made a mistake), you can perform a hardware factory reset.

1. Press and **hold both Key 0 (far left) and Key 3 (far right)** simultaneously.
2. While holding the keys, press the tiny `RESET` button on the Xiao ESP32-S3 microcontroller, or unplug and replug the USB power.
3. Keep holding Key 0 and Key 3 for about **3 seconds** until the LEDs flash RED.
4. Release the keys. The device will completely wipe its internal memory and reboot into setup mode.
