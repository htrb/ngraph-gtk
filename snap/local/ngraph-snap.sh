#! /bin/sh

RUBY_VERSION="2.5.0"
RUBYLIB="$SNAP/usr/lib/ruby/vendor_ruby/$RUBY_VERSION:$SNAP/usr/lib/$SNAP_LAUNCHER_ARCH_TRIPLET/ruby/vendor_ruby/$RUBY_VERSION:$SNAP/usr/lib/ruby/vendor_ruby:$SNAP/usr/lib/ruby/$RUBY_VERSION:$SNAP/usr/lib/$SNAP_LAUNCHER_ARCH_TRIPLET/ruby/$RUBY_VERSION"
export RUBYLIB
exec "$SNAP"/usr/bin/ngraph "$@"
