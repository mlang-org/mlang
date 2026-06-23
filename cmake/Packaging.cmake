# Native installer packaging for the MLang toolchain via CPack.
#
# Produces .deb and .rpm on Linux and a WiX .msi on Windows. The toolchain is
# split into selectable components:
#   - core  : mlangc, runtime, mfmt, mls, mbuild, the standard library (required)
#   - mango : the package manager (optional)
# On Windows the MSI presents these as features the user can select; on Linux
# each component is a separate package so users can choose to install mango.

set(CPACK_PACKAGE_NAME "mlang")
set(CPACK_PACKAGE_VENDOR "The MLang Project")
set(CPACK_PACKAGE_CONTACT "MLang Maintainers <maintainers@mlang.org>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The MLang programming language toolchain")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/mlang-org/mlang")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MLang")
# WiX requires a .txt/.rtf license; the repo LICENSE has no extension, so copy it.
configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LICENSE" "${CMAKE_BINARY_DIR}/LICENSE.txt" COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_STRIP_FILES ON)

# Component-based packaging.
set(CPACK_DEB_COMPONENT_INSTALL ON)
set(CPACK_RPM_COMPONENT_INSTALL ON)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

if(MLANG_HAVE_MANGO)
    set(CPACK_COMPONENTS_ALL core mango)
else()
    set(CPACK_COMPONENTS_ALL core)
endif()

set(CPACK_COMPONENT_CORE_DISPLAY_NAME "MLang toolchain")
set(CPACK_COMPONENT_CORE_DESCRIPTION
    "The compiler (mlangc), runtime, mfmt, mls, mbuild, and the standard library.")
set(CPACK_COMPONENT_CORE_REQUIRED ON)

set(CPACK_COMPONENT_MANGO_DISPLAY_NAME "mango package manager")
set(CPACK_COMPONENT_MANGO_DESCRIPTION
    "The mango package manager for fetching and publishing MLang packages.")

# Descriptive artifact names (mlang_0.1.0_amd64.deb, mlang-0.1.0.x86_64.rpm).
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)

# --- Debian ----------------------------------------------------------------
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_CORE_PACKAGE_NAME "mlang")
set(CPACK_DEBIAN_MANGO_PACKAGE_NAME "mlang-mango")
set(CPACK_DEBIAN_CORE_PACKAGE_RECOMMENDS "mlang-mango")

# --- RPM -------------------------------------------------------------------
set(CPACK_RPM_PACKAGE_LICENSE "MIT")
set(CPACK_RPM_PACKAGE_GROUP "Development/Languages")
set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_RPM_CORE_PACKAGE_NAME "mlang")
set(CPACK_RPM_MANGO_PACKAGE_NAME "mlang-mango")

# --- Windows (WiX -> .msi, with feature selection) -------------------------
set(CPACK_WIX_UPGRADE_GUID "7C2A9D14-3E48-4A6B-9F10-2B3C4D5E6F70")
set(CPACK_WIX_PROPERTY_ARPHELPLINK "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_WIX_ROOT_FEATURE_TITLE "MLang Toolchain")

include(CPack)
