# agent_functions/ls.py

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

class LsArguments(TaskArguments):
    def __init__(self, command_line, **kwargs):
        super().__init__(command_line, **kwargs)
        self.args = [
            CommandParameter(
                name="path",
                cli_name="path",
                display_name="Path",
                type=ParameterType.String,
                description="Ruta a listar (por defecto: '.')",
                parameter_group_info=[ParameterGroupInfo(ui_position=1, required=False)],
            )
        ]

    async def parse_arguments(self):
        if self.command_line and not self.command_line.startswith("{"):
            self.add_arg("path", self.command_line)
        else:
            self.load_args_from_json_string(self.command_line)

class LsCommand(CommandBase):
    cmd              = "ls"
    needs_admin      = False
    help_cmd         = "ls [path]"
    description      = "Lista el contenido de un directorio"
    version          = 1
    author           = "Ruben"
    argument_class   = LsArguments
    attackmapping    = ["T1083"]
    supported_ui_features = ["callback_table:ls"]  
    attributes       = CommandAttributes(
        supported_os=[SupportedOS.Windows],
        builtin=True,
        suggested_command=True,
    )

    async def create_tasking(self, task: LsArguments) -> LsArguments:
        task.display_params = task.args.get_arg("path") or "."
        return task

    async def process_response(self, response: AgentResponse) -> AgentResponse:
        return response
