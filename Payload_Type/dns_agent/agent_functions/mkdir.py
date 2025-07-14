# agent_functions/mkdir.py

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

class MkdirArguments(TaskArguments):
    def __init__(self, command_line, **kwargs):
        super().__init__(command_line, **kwargs)
        self.args = [
            CommandParameter(
                name="directory",
                cli_name="directory",
                display_name="Directory",
                type=ParameterType.String,
                description="Directorio a crear",
                parameter_group_info=[ParameterGroupInfo(ui_position=1, required=True)],
            )
        ]

    async def parse_arguments(self):
        if self.command_line and not self.command_line.startswith("{"):
            self.add_arg("directory", self.command_line)
        else:
            self.load_args_from_json_string(self.command_line)

class MkdirCommand(CommandBase):
    cmd              = "mkdir"
    needs_admin      = False
    help_cmd         = "mkdir <directory>"
    description      = "Crea un nuevo directorio"
    version          = 1
    author           = "Ruben"
    argument_class   = MkdirArguments
    attackmapping    = ["T1106"]
    supported_ui_features = ["callback_table:mkdir"]  
    attributes       = CommandAttributes(
        supported_os=[SupportedOS.Windows],
        builtin=True,
        suggested_command=True,
    )

    async def create_tasking(self, task: MkdirArguments) -> MkdirArguments:
        task.display_params = task.args.get_arg("directory")
        return task

    async def process_response(self, response: AgentResponse) -> AgentResponse:
        return response
