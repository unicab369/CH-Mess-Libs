import pywinusb.hid as hid
import usb.core
import usb.util


def clear_hid_feature():
    # Find your device
    target_vid, target_pid = 0x413d, 0x2107
    devices = hid.find_all_hid_devices()
    
    target_device = None
    for device in devices:
        if device.vendor_id == target_vid and device.product_id == target_pid:
            target_device = device
            print(f"✅ Found device: {device.product_name}")
            break
    
    if target_device is None:
        print("❌ Device not found")
        return
    
    try:
        # Open the device
        target_device.open()
        print("📱 Device opened")
        
        # HID devices use feature reports instead of CLEAR_FEATURE
        # Send a feature report to clear/reset the device
        feature_report_data = [0x00] * 65  # 64 bytes + report ID (if needed)
        
        # Try to send feature report (this is HID equivalent)
        target_device.send_feature_report(feature_report_data)
        print("✅ Feature report sent (HID equivalent of CLEAR_FEATURE)")
        
        # Close device
        target_device.close()
        print("📱 Device closed")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        if target_device.is_opened():
            target_device.close()


if __name__ == "__main__":
    print("Testing HID devices...")
    devices = hid.find_all_hid_devices()
    print(f"Found {len(devices)} devices")

    for dev in devices:
        print(f"- {dev.product_name} (Vendor: 0x{dev.vendor_id:04x})")

    # clear_hid_feature()

    devices = usb.core.find(find_all=True)
    for dev in devices:
        print(f"Device: VID=0x{dev.idVendor:04x}, PID=0x{dev.idProduct:04x}")
        try:
            print(f"  Manufacturer: {usb.util.get_string(dev, dev.iManufacturer)}")
            print(f"  Product: {usb.util.get_string(dev, dev.iProduct)}")
            print(f"  Serial: {usb.util.get_string(dev, dev.iSerialNumber)}")
        except usb.core.USBError:
            print("  (Cannot read strings — maybe need admin rights)")
        print("-" * 50)
