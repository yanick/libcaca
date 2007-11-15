require 'test/unit'
require 'cucul'

class TC_Canvas < Test::Unit::TestCase
    def setup
        @c = Cucul::Canvas.new(3, 3)
    end
    def test_create
        c = Cucul::Canvas.new(3, 3)
        assert_not_nil(c, 'Canvas creation failed')
        assert(c.width == 3 && c.height == 3, 'Wrong size for new canvas')
    end
    def test_width
        @c.width = 42
        assert_equal(42, @c.width, 'Failed to set width with =')
        @c.set_width(24)
        assert_equal(24, @c.width, 'Failed to set width')
    end
    def test_height
        @c.height = 42
        assert_equal(42, @c.height, 'Failed to set height with =')
        @c.set_height(24)
        assert_equal(24, @c.height, 'Failed to set height')
    end
    def test_size
        @c.set_size(100,100)
        assert(@c.width == 100 && @c.height == 100, 'Failed to set size')
    end
    def test_import
        @c.import_memory("foo", "")
        assert_equal("foo\r\n", @c.export_memory("irc"), "Import/Export failed")
    end
    def test_cursor
        @c.gotoxy(1,1)
        assert_equal(1, @c.cursor_x)
        assert_equal(1, @c.cursor_y)
    end
    def test_clear
        @c.put_char(1, 1, 64)
        @c.clear
        assert_equal("", @c.export_memory("irc").strip, "Failed to clear canvas")
    end
    def test_char
        @c.put_char(1, 1, 42)
        assert_equal(42, @c.get_char(1,1))
    end
end
