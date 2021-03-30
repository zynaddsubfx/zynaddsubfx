# Documentation: https://docs.brew.sh/Formula-Cookbook
#                https://rubydoc.brew.sh/Formula
# PLEASE REMOVE ALL GENERATED COMMENTS BEFORE SUBMITTING YOUR PULL REQUEST!
class Zynaddsubfx < Formula
  desc "Fully featured musical software synthesizer for Linux, MacOS, BSD, and Windows"
  homepage "https://zynaddsubfx.sf.net"
  url "https://github.com/zynaddsubfx/zynaddsubfx/archive/refs/heads/master.zip"
  version "3.0.6-rc3"
  sha256 "2ccc75c6118b833895e955aa5ba14fa860e6cad12e4d09ba41c39cae57cde6fa"
  license "GPLv2"

  depends_on "cmake" => :build
  depends_on "fftw"
  depends_on "libmxml"
  depends_on "zlib"
  depends_on "liblo"

  def install
    # ENV.deparallelize  # if your formula fails when building in parallel
    # Remove unrecognized options if warned by configure
    # https://rubydoc.brew.sh/Formula.html#std_configure_args-instance_method
    # system "./configure", *std_configure_args, "--disable-silent-rules"
    system "git", "clone", "https://github.com/DISTRHO/DPF.git"
    system "git", "clone", "https://github.com/zynaddsubfx/instruments.git"
    system "git", "clone", "https://github.com/fundamental/rtosc.git" 
    system "cmake", ".", *std_cmake_args
    system "make", "-j8"
    system "make", "install"
  end

  test do
    # `test do` will create, run in and delete a temporary directory.
    #
    # This test will fail and we won't accept that! For Homebrew/homebrew-core
    # this will need to be a test that verifies the functionality of the
    # software. Run the test with `brew test zynaddsubfx`. Options passed
    # to `brew install` such as `--HEAD` also need to be provided to `brew test`.
    #
    # The installed folder is not in the path, so use the entire path to any
    # executables being tested: `system "#{bin}/program", "do", "something"`.
    system "false"
  end
end
