# agent_functions/cd.py

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

class CdArguments(TaskArguments):
    def __init__(self, command_line, **kwargs):
        super().__init__(command_line, **kwargs)
        self.args = [
            CommandParameter(
                name="directory",
                cli_name="directory",
                display_name="Directory",
                type=ParameterType.String,
                description="Directorio al que cambiar",
                parameter_group_info=[ParameterGroupInfo(ui_position=1, required=True)],
            )
        ]

    async def parse_arguments(self):
        if self.command_line and not self.command_line.startswith("{"):
            # argumento directo
            self.add_arg("directory", self.command_line)
        else:
            # JSON
            self.load_args_from_json_string(self.command_line)

class CdCommand(CommandBase):
    cmd              = "cd"
    needs_admin      = False
    help_cmd         = "cd <directory>"
    description      = "Cambia el directorio de trabajo"
    version          = 1
    author           = "Ruben"
    argument_class   = CdArguments
    attackmapping    = ["T1106"]
    supported_ui_features = ["callback_table:cd"]  
    attributes       = CommandAttributes(
        supported_os=[SupportedOS.Windows],
        builtin=True,
        suggested_command=True,
    )

    async def create_tasking(self, task: CdArguments) -> CdArguments:
        task.display_params = task.args.get_arg("directory")
        return task

    async def process_response(self, response: AgentResponse) -> AgentResponse:
        return response
