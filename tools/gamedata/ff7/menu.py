import struct

from . import lzss
from . import ff7text

class ShopEntry:
    def __init__(self, itemType, itemID):
        self.isMateria = (itemType == 1)
        self.id = itemID

class Shop:
    def __init__(self, data, offset):
        self.items = []

        inv_count = data[offset + 2]
        for i in range(0, inv_count):
            item_addr = offset + 4 + (i * 8)
            itemType, itemID = struct.unpack_from("<IH", data, item_addr)
            self.items.append(ShopEntry(itemType, itemID))

# Shop Menu data file
class ShopMenuData:

    # Parse the shop data from an open file object.
    def __init__(self, fileobj):

        # Read the file data
        data = fileobj.read()

        item_count = 320
        self.item_prices = struct.unpack_from(f"<{item_count}I", data, 0x6854)

        materia_count = 91
        self.materia_prices = struct.unpack_from(f"<{materia_count}I", data, 0x6E54)

        shop_count = 80
        shop_start = 0x4714
        self.shops = []

        for id in range(0, shop_count):
            shop_addr = shop_start + (id * 84)
            self.shops.append(Shop(data, shop_addr))


# Limit Menu data file
class LimitMenuData:

    # Parse the shop data from an open file object.
    def __init__(self, fileobj):
        data = fileobj.read()

        record_start = 0x1324
        numAttacks = 71

        # 28 bytes per record as established
        fmt = "< I B I B B H B B B B H B I H H"
        record_size = struct.calcsize(fmt)
        
        # Simple bounds check
        if len(data) < (record_size * numAttacks):
            raise ValueError(f"Buffer too small. Expected {record_size * numAttacks} bytes.")
        
        self.attacks = []
        
        for i in range(numAttacks):
            # Calculate the start and end indices for the current record
            start = record_start + (i * record_size)
            end = start + record_size
            
            # Slice and unpack
            record_bytes = data[start:end]
            unpacked = struct.unpack(fmt, record_bytes)
            
            self.attacks.append({
                "unknown0":         unpacked[0],
                "casting_cost":     unpacked[1],
                "unknown1":         unpacked[2],
                "unknown2":         unpacked[3],
                "attack_type":      unpacked[4],
                "attack_attribute": unpacked[5],
                "id_number":        unpacked[6],
                "restore_apply":    unpacked[7],
                "strength":         unpacked[8],
                "restore_type":     unpacked[9],
                "unknown3":         unpacked[10],
                "times_attacking":  unpacked[11],
                "statuses":         unpacked[12],
                "element":          unpacked[13],
                "unknown4":         unpacked[14],
            })