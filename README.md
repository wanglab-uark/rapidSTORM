# About rapidSTORM

rapidSTORM is a localization microscopy evaluation program. 
It is designed to find and precisely localize fluorophores in 
super-resolvable movies such as those generated by the PALM or
dSTORM techniques.

# Resources

  * [Online manual](https://storage.googleapis.com/rapidstorm/doc/index.html)
  * [Installation & Downloads](https://stevewolter.github.io/rapidSTORM/)
  * [Mailing list](https://groups.google.com/forum/#!forum/rapidstorm-discuss)
  * [Source code](https://github.com/stevewolter/rapidSTORM)

# Compilation from source

Compilation from source requires the following libraries, which are available from Debian and the Web:

  * [Eigen 3](http://eigen.tuxfamily.org)
  * [Boost 1.46](http://www.boost.org)
  * [GraphicsMagick 1](http://www.graphicsmagick.org/)
  * [libtiff 4](https://www.libtiff.org)
  * [wxWidgets 2.8](https://www.wxwidgets.org)
  * [GNU Scientific Library](http://www.gnu.org/s/gsl)
  * [tinyxml](http://www.grinninglizard.com/tinyxml)

Additionally, a few lesser-known libraries are optional. We have packaged
all of these libraries and made them available from our homepage at
https://stevewolter.github.io/rapidSTORM/:

  * libreadsif 1.4: Library maintained by the rapidSTORM team based on code kindly released by Andor technologies for reading Andor SIF files.
  * libb64: Base64 encoding/decoding library by Chris Venter

On Debian or Ubuntu systems, you can add our repository to the sources as described on the
website, and install all dependencies as:

    $ sudo apt-get build-dep rapidstorm

If you have cloned the git repository (instead of using the release .tar.gz), you need to
run autoreconf to create the configure script. This requires the autoconf-archive scripts.

    $ sudo apt-get install autoconf-archive
    # In the source directory:
    $ autoreconf --install
