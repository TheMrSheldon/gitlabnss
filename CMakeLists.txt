cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(NSSGITLAB_VERSION 0.0.1)

project(nssgitlab VERSION ${NSSGITLAB_VERSION} LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(src)

##########################################################################################
# Debian Package
##########################################################################################
# Debian Packaging only available if this is the main app
if((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME))
	include(GNUInstallDirs)

	install(TARGETS nss_gitlab
		ARCHIVE DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}
			COMPONENT nssgitlab
        PERMISSIONS PERMISSIONS OWNER_READ OWNER_WRITE #?? GROUP_READ GROUP_WRITE WORLD_READ WORLD_WRITE
    )
    install(TARGETS authorized_keys gitlabnssd
        RUNTIME DESTINATION "/bin" # ${CMAKE_INSTALL_FULL_BINDIR}
            COMPONENT nssgitlab
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    )
    install(FILES res/gitlabnssd
        DESTINATION "/etc/init.d"
            COMPONENT nssgitlab
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
    )
    install(FILES "res/gitlabnss.conf"
        DESTINATION "/etc/gitlabnss"
            COMPONENT nssgitlab
        PERMISSIONS OWNER_READ GROUP_READ WORLD_READ
    )

	SET(CPACK_GENERATOR "DEB")
	SET(CPACK_PACKAGE_NAME "nssgitlab")
	SET(CPACK_SET_DESTDIR TRUE)
	SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "not-set")
	SET(CPACK_PACKAGE_VERSION ${NSSGITLAB_VERSION})
	SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "GitLab login for NSS")
	# SET(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
	# SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
	SET(CPACK_DEBIAN_PACKAGE_DEPENDS "")
	SET(CPACK_PACKAGE_VENDOR "")
	include(CPack)
	cpack_add_component(nssgitlab)
endif()