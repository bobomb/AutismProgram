################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
AutismProgram.obj: ../AutismProgram.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmspx --abi=eabi --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/Users/Mo/Documents/Code Composer/AutismProgram/HAL" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --define=MHZ_868 --define=__CC430F6137__ --diag_warning=225 --silicon_errata=CPU18 --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="AutismProgram.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

Button.obj: ../Button.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmspx --abi=eabi --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/Users/Mo/Documents/Code Composer/AutismProgram/HAL" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --define=MHZ_868 --define=__CC430F6137__ --diag_warning=225 --silicon_errata=CPU18 --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="Button.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

RadioControl.obj: ../RadioControl.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430_4.2.1/bin/cl430" -vmspx --abi=eabi --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/Users/Mo/Documents/Code Composer/AutismProgram/HAL" --include_path="C:/ti/ccsv5/tools/compiler/msp430_4.2.1/include" -g --define=MHZ_868 --define=__CC430F6137__ --diag_warning=225 --silicon_errata=CPU18 --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="RadioControl.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


