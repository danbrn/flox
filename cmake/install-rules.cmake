install(
    TARGETS flox_exe
    RUNTIME COMPONENT flox_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
