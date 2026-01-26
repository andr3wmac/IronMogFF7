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