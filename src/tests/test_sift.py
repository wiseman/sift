#!/usr/bin/env python

import unittest
import ctypes

import sift


class SIFTDatabaseTests(unittest.TestCase):
    def test_add(self):
        "Adding image."
        db = sift.Database()
        self.assert_(not db.contains_label('test1'))
        self.assertEqual(db.image_count(), 0)
        self.assertEqual(db.feature_count(), 0)

        db.add_image_file('duckies-person.jpg', 'test1')
        self.assert_(db.contains_label('test1'))
        self.assertEqual(db.image_count(), 1)
        self.assert_(db.feature_count() > 0)

        db.add_image_file('duckies-person.jpg', 'test2')
        self.assert_(db.contains_label('test2'))
        self.assertEqual(db.image_count(), 2)
        self.assert_(db.feature_count() > 0)
        
        self.assertRaises(sift.SIFTError,
                          lambda: db.add_image_file('duckies-person.jpg', 'test2'))
        self.assertRaises(Exception,
                          lambda: db.add_image_file('non-existent-image.jpg', 'woo'))
        self.assertRaises(ctypes.ArgumentError,
                          lambda: db.add_image_file({}, 'woo2'))
        self.assertRaises(ctypes.ArgumentError,
                          lambda: db.contains_label(5))

        def test_db_duckies(db):
            db.add_image_file('duckies-shirt.jpg', 'test1')
            self.assert_(db.contains_label('test1'))
            result = db.match_image_file('duckies-person.jpg')
            self.assertEqual(len(result), 1)
            self.assertEqual(result[0].label, 'test1')

        db = sift.Database()
        test_db_duckies(db)

    def test_remove_image(self):
        "Removing images"
        db = sift.Database()
        db.add_image_file('duckies-person.jpg', 'test1')
        db.add_image_file('duckies-person.jpg', 'test2')
        db.add_image_file('duckies-person.jpg', 'test3')
        self.assert_(db.contains_label('test1') and db.contains_label('test2') \
                         and db.contains_label('test3'))
        self.assert_(db.feature_count() > 0)
        self.assertEqual(db.image_count(), 3)

        self.assertEqual(db.remove_image('test1'), True)
        self.assert_((not db.contains_label('test1')) and db.contains_label('test2') \
                         and db.contains_label('test3'))
        self.assert_(db.feature_count() > 0)
        self.assertEqual(db.image_count(), 2)
        
        self.assertEqual(db.remove_image('test2'), True)
        self.assert_((not db.contains_label('test1')) and (not db.contains_label('test2')) \
                         and db.contains_label('test3'))
        self.assert_(db.feature_count() > 0)
        self.assertEqual(db.image_count(), 1)
        
        self.assertEqual(db.remove_image('test3'), True)
        self.assert_((not db.contains_label('test1')) and (not db.contains_label('test2')) \
                         and (not db.contains_label('test3')))
        self.assertEqual(db.feature_count(), 0)
        self.assertEqual(db.image_count(), 0)
        
        self.assertEqual(db.remove_image('non-existent-label'), False)
        self.assertRaises(ctypes.ArgumentError,
                          lambda: db.remove_image(5))



unittest.main()

if __name__ == '__main__':
    unittest.main()
