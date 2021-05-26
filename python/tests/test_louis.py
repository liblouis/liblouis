import unittest
import louis

class TestFaceWithTearsOfJoy(unittest.TestCase):

    @unittest.expectedFailure
    def test_1(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "😂"),
                         '<face with tears of joy>')

    @unittest.expectedFailure
    def test_2(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "a 😂"),
                         'a <face with tears of joy>')

    @unittest.expectedFailure
    def test_3(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "😂 b"),
                         '<face with tears of joy> b')

    @unittest.expectedFailure
    def test_4(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "a 😂 b"),
                         'a <face with tears of joy> b')

    @unittest.expectedFailure
    def test_8(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "before 😂"),
                         'before <face with tears of joy>')

    @unittest.expectedFailure
    def test_9(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "😂 after"),
                         '<face with tears of joy> after')

    @unittest.expectedFailure
    def test_10(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "before 😂 after"),
                         'before <face with tears of joy> after')

    @unittest.expectedFailure
    def test_11(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "🤣"),
                         '<rolling on the floor laughing>')

    @unittest.expectedFailure
    def test_12(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "a 🤣"),
                         'a <rolling on the floor laughing>')

    @unittest.expectedFailure
    def test_13(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "🤣 b"),
                         '<rolling on the floor laughing> b')

    @unittest.expectedFailure
    def test_14(self):
        self.assertEqual(louis.translateString(["en-us-g2.ctb", "tests/test.cti"], "a 🤣 b"),
                         'a <rolling on the floor laughing> b')

if __name__ == '__main__':
    unittest.main()
