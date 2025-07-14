# agent_functions/shell.py

from mythic_container.MythicCommandBase import (
    CommandBase,
    TaskArguments,
    CommandParameter,
    ParameterType,
    ParameterGroupInfo,
    CommandAttributes,
    SupportedOS,
    MythicTask,
    AgentResponse,
)

class ShellArguments(TaskArguments):
    def __init__(self, command_line, **kwargs):
        super().__init__(command_line, **kwargs)
        self.args = [
            CommandParameter(
                name="cmd",
                cli_name="cmd",
                display_name="Command",
                type=ParameterType.String,
                description="Comando a ejecutar en shell",
                parameter_group_info=[ParameterGroupInfo(ui_position=1, required=True)],
            )
        ]

    async def parse_arguments(self):
        if self.command_line and not self.command_line.startswith("{"):
            self.add_arg("cmd", self.command_line)
        else:
            self.load_args_from_json_string(self.command_line)

class ShellCommand(CommandBase):
    cmd              = "shell"
    needs_admin      = False
    help_cmd         = "shell <cmd>"
    description      = "Ejecuta un comando en cmd.exe"
    version          = 1
    author           = "Ruben"
    argument_class   = ShellArguments
    attackmapping    = ["T1059"]
    supported_ui_features = ["callback_table:shell"]
    attributes       = CommandAttributes(
        supported_os=[SupportedOS.Windows],
        builtin=True,
        suggested_command=True,
    )

    async def create_tasking(self, task: ShellArguments) -> ShellArguments:
        task.display_params = task.args.get_arg("cmd")
        return task

    async def process_response(self, response: AgentResponse) -> AgentResponse:
        return response
