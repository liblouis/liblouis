import unittest
import louis

class TestUnicodeComposed(unittest.TestCase):

    @unittest.expectedFailure
    def test_1(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "😂"),
                         '"<face with tears of joy">')

    def test_2(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "a 😂"),
                         'a "<face with tears of joy">')

    def test_3(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "😂 b"),
                         '"<face with tears of joy"> b')

    def test_4(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "a 😂 b"),
                         'a "<face with tears of joy"> b')

    def test_8(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "before 😂"),
                         'before "<face with tears of joy">')

    def test_9(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "😂 after"),
                         '"<face with tears of joy"> after')

    def test_10(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "before 😂 after"),
                         'before "<face with tears of joy"> after')

    @unittest.expectedFailure
    def test_11(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "🤣"),
                         '"<rolling on the floor laughing">')

    def test_12(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "a 🤣"),
                         'a "<rolling on the floor laughing">')

    def test_13(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "🤣 b"),
                         '"<rolling on the floor laughing"> b')

    def test_14(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "a 🤣 b"),
                         'a "<rolling on the floor laughing"> b')

class TestUnicodeDecomposed(unittest.TestCase):

    @unittest.expectedFailure
    def test_1(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "\ud83d\ude02"),
                         '"<face with tears of joy">')

    def test_2(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "a \ud83d\ude02"),
                         'a "<face with tears of joy">')

    def test_3(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "\ud83d\ude02 b"),
                         '"<face with tears of joy"> b')

    def test_4(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "a \ud83d\ude02 b"),
                         'a "<face with tears of joy"> b')

    def test_8(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "before \ud83d\ude02"),
                         'before "<face with tears of joy">')

    def test_9(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "\ud83d\ude02 after"),
                         '"<face with tears of joy"> after')

    def test_10(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "before \ud83d\ude02 after"),
                         'before "<face with tears of joy"> after')

    @unittest.expectedFailure
    def test_11(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "\ud83e\udd23"),
                         '"<rolling on the floor laughing">')

    def test_12(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "a \ud83e\udd23"),
                         'a "<rolling on the floor laughing">')

    def test_13(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "\ud83e\udd23 b"),
                         '"<rolling on the floor laughing"> b')

    def test_14(self):
        self.assertEqual(louis.translateString(["en-ueb-g1.ctb", "tests/test.cti"], "a \ud83e\udd23 b"),
                         'a "<rolling on the floor laughing"> b')

class TestEndianness(unittest.TestCase):
    def test_1(self):
        self.assertEqual(louis.translate(["unicode.dis","en-chardefs.cti"], "abcdefghijklmnopqrstuvwxyz")[0],
                         "⠁⠃⠉⠙⠑⠋⠛⠓⠊⠚⠅⠇⠍⠝⠕⠏⠟⠗⠎⠞⠥⠧⠺⠭⠽⠵")

    def test_2(self):
        # invert encoding
        _encoding = louis.conversionEncoding
        _endianness = "le" if louis._endianness == "be" else "be"
        louis.conversionEncoding = "utf_%d_%s" % (louis.wideCharBytes * 8, _endianness)
        with self.assertRaises(UnicodeDecodeError) as context:
            self.assertEqual(louis.translate(["unicode.dis","en-chardefs.cti"], "abcdefghijklmnopqrstuvwxyz")[0],
                             "⠁⠃⠉⠙⠑⠋⠛⠓⠊⠚⠅⠇⠍⠝⠕⠏⠟⠗⠎⠞⠥⠧⠺⠭⠽⠵")
        louis.conversionEncoding = _encoding

if __name__ == '__main__':
    unittest.main()
